#include "logrecordwindow.h"
#include "ui_logrecord_window.h"

#include <QSqlTableModel>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QFrame>
#include <QDate>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDateTime>
#include <QVector>
#include <QDir>
#include <QPainter>

#include <algorithm>

// QtCharts
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

#include <QSqlQuery>

#include "dbmanager.h"
#include "addlogwindow.h"

namespace {

class LogFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit LogFilterProxyModel(QObject* parent=nullptr) : QSortFilterProxyModel(parent) {}

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        const QString kw = filterRegularExpression().pattern().trimmed();
        if (kw.isEmpty()) return true;

        // title/person/place/date/score 全字段模糊匹配
        const int cols = sourceModel()->columnCount();
        for (int c = 0; c < cols; ++c) {
            const QModelIndex idx = sourceModel()->index(source_row, c, source_parent);
            const QString v = sourceModel()->data(idx).toString();
            if (v.contains(kw, Qt::CaseInsensitive)) return true;
        }
        return false;
    }
};

static int safeToInt(const QString& s, int def)
{
    bool ok=false;
    int v=s.trimmed().toInt(&ok);
    return ok ? v : def;
}

}

LogRecordWindow::LogRecordWindow(int userId, const QString& username, const QString& role, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LogRecordWindow)
    , m_userId(userId)
    , m_username(username)
    , m_role(role)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
    refreshData();
}

LogRecordWindow::~LogRecordWindow()
{
    delete ui;
}

void LogRecordWindow::initUi()
{
    // 让 tableView 刚好填满 frame（去掉布局默认边距）
    if (ui->horizontalLayout_4) {
        ui->horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        ui->horizontalLayout_4->setSpacing(0);
    }

    // tableView 基础设置
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setFrameShape(QFrame::NoFrame);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    // model
    m_model = new QSqlTableModel(this, DbManager::instance().database());
    m_model->setTable("logs");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    if (m_role != "admin") {
        m_model->setFilter(QString("user_id=%1").arg(m_userId));
    } else {
        m_model->setFilter(QString());
    }
    m_model->select();

    // headers
    // logs: id,user_id,title,person,place,log_date,mood_score,created_at
    m_model->setHeaderData(2, Qt::Horizontal, "事件标题");
    m_model->setHeaderData(3, Qt::Horizontal, "相关人物");
    m_model->setHeaderData(4, Qt::Horizontal, "地点/备注");
    m_model->setHeaderData(5, Qt::Horizontal, "日期");
    m_model->setHeaderData(6, Qt::Horizontal, "情绪评分");
    m_model->setHeaderData(7, Qt::Horizontal, "创建时间");
    m_model->setHeaderData(1, Qt::Horizontal, "用户ID");

    m_proxy = new LogFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);

    ui->tableView->setModel(m_proxy);
    // 隐藏 id/user_id
    ui->tableView->hideColumn(0);
    if (m_role != "admin") {
        ui->tableView->hideColumn(1);
    }

    updateCountLabel();
}

void LogRecordWindow::connectSignals()
{
    connect(ui->add_log_pushButton, &QPushButton::clicked, this, &LogRecordWindow::onAddLogClicked);
    connect(ui->refresh_pushButton, &QPushButton::clicked, this, &LogRecordWindow::onRefreshClicked);
    connect(ui->export_csv_pushButton, &QPushButton::clicked, this, &LogRecordWindow::onExportCsvClicked);
    connect(ui->search_log_pushButton, &QPushButton::clicked, this, &LogRecordWindow::onSearchClicked);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &LogRecordWindow::onTableContextMenuRequested);

    // Charts refresh when table changes
    if (m_model) {
        connect(m_model, &QAbstractItemModel::dataChanged, this, &LogRecordWindow::updateChartsFromTable);
        connect(m_model, &QAbstractItemModel::modelReset, this, &LogRecordWindow::updateChartsFromTable);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, &LogRecordWindow::updateChartsFromTable);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, &LogRecordWindow::updateChartsFromTable);
    }
}

void LogRecordWindow::refreshData()
{
    if (!m_model) return;
    if (m_role != "admin") {
        m_model->setFilter(QString("user_id=%1").arg(m_userId));
    } else {
        m_model->setFilter(QString());
    }
    m_model->select();
    updateCountLabel();

    initChartsIfNeeded();
    updateChartsFromTable();
}

void LogRecordWindow::updateCountLabel()
{
    if (!m_proxy) return;
    ui->book_total_label->setText(QString("总计：%1").arg(m_proxy->rowCount()));
}

void LogRecordWindow::initChartsIfNeeded()
{
    if (m_pieChartView && m_lineChartView) {
        return;
    }

    // Top: Pie chart (事件类聚)
    if (!m_pieChartView && ui->pie_widget) {
        auto* lay = new QVBoxLayout(ui->pie_widget);
        lay->setContentsMargins(0, 0, 0, 0);

        auto* chart = new QChart();
        chart->setTitle(QStringLiteral("事件类聚（按标题次数）"));
        chart->legend()->setAlignment(Qt::AlignRight);

        m_pieChartView = new QChartView(chart, ui->pie_widget);
        m_pieChartView->setRenderHint(QPainter::Antialiasing);
        m_pieChartView->setRubberBand(QChartView::NoRubberBand);
        lay->addWidget(m_pieChartView);
    }

    // Bottom: Line chart (时光折线铺)
    if (!m_lineChartView && ui->line_widget) {
        auto* lay = new QVBoxLayout(ui->line_widget);
        lay->setContentsMargins(0, 0, 0, 0);

        auto* chart = new QChart();
        chart->setTitle(QStringLiteral("时光折线铺（按日期事件数）"));
        chart->legend()->hide();

        m_lineChartView = new QChartView(chart, ui->line_widget);
        m_lineChartView->setRenderHint(QPainter::Antialiasing);
        m_lineChartView->setRubberBand(QChartView::NoRubberBand);
        lay->addWidget(m_lineChartView);
    }
}

void LogRecordWindow::updateChartsFromTable()
{
    if (!m_proxy) return;
    if (!m_pieChartView || !m_lineChartView) return;

    // Aggregate from the *currently displayed* rows (proxy model):
    // columns (source): 2=title, 5=log_date
    QMap<QString, int> titleCounts;
    QMap<QDate, int> dateCounts;

    const int rows = m_proxy->rowCount();
    for (int r = 0; r < rows; ++r) {
        const QString title = m_proxy->data(m_proxy->index(r, 2)).toString().trimmed();
        const QString dateStr = m_proxy->data(m_proxy->index(r, 5)).toString().trimmed();

        if (!title.isEmpty()) {
            titleCounts[title] += 1;
        }

        QDate d = QDate::fromString(dateStr, Qt::ISODate);
        if (!d.isValid()) d = QDate::fromString(dateStr, "yyyy/M/d");
        if (!d.isValid()) d = QDate::fromString(dateStr, "yyyy-MM-dd");
        if (d.isValid()) {
            dateCounts[d] += 1;
        }
    }

    // ----------------- Pie chart -----------------
    {
        QChart* chart = m_pieChartView->chart();
        chart->removeAllSeries();

        auto* series = new QPieSeries(chart);

        // Take top N titles, rest -> "其他"
        QVector<QPair<QString, int>> vec;
        vec.reserve(titleCounts.size());
        for (auto it = titleCounts.constBegin(); it != titleCounts.constEnd(); ++it) {
            vec.append({it.key(), it.value()});
        }
        std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

        const int topN = 8;
        int other = 0;
        for (int i = 0; i < vec.size(); ++i) {
            if (i < topN) {
                auto* slice = series->append(vec[i].first, vec[i].second);
                slice->setLabel(QString("%1 (%2)").arg(vec[i].first).arg(vec[i].second));
                slice->setLabelVisible(true);
            } else {
                other += vec[i].second;
            }
        }
        if (other > 0) {
            auto* slice = series->append(QStringLiteral("其他"), other);
            slice->setLabel(QString("其他 (%1)").arg(other));
            slice->setLabelVisible(true);
        }

        chart->addSeries(series);
    }

    // ----------------- Line chart -----------------
    {
        QChart* chart = m_lineChartView->chart();
        chart->removeAllSeries();

        // Remove old axes
        const auto axes = chart->axes();
        for (auto* ax : axes) {
            chart->removeAxis(ax);
            ax->deleteLater();
        }

        auto* series = new QLineSeries(chart);

        QList<QDate> dates = dateCounts.keys();
        std::sort(dates.begin(), dates.end());

        int maxY = 0;
        for (const QDate& d : dates) {
            const int c = dateCounts.value(d);
            maxY = std::max(maxY, c);
            const QDateTime dt(d, QTime(0, 0, 0));
            series->append(dt.toMSecsSinceEpoch(), c);
        }

        chart->addSeries(series);

        auto* axisX = new QDateTimeAxis(chart);
        axisX->setFormat("MM-dd");
        axisX->setTitleText(QStringLiteral("日期"));
        const int nDates = static_cast<int>(dates.size());
        axisX->setTickCount(std::min(10, std::max(2, nDates)));
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        auto* axisY = new QValueAxis(chart);
        axisY->setTitleText(QStringLiteral("事件数"));
        axisY->setLabelFormat("%d");
        axisY->setRange(0, std::max(1, maxY + 1));
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    }
}

void LogRecordWindow::onAddLogClicked()
{
    AddLogWindow dlg(this);
    dlg.setWindowTitle("添加事件记录");

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const QString title = dlg.getTitle();
    const QString person = dlg.getPerson();
    const QString place = dlg.getPlace();
    QString dateStr = dlg.getDateStr();
    int score = dlg.getMoodScore();

    if (dateStr.trimmed().isEmpty()) {
        dateStr = QDate::currentDate().toString("yyyy-MM-dd");
    } else {
        // 尝试规范化为 yyyy-MM-dd
        const QDate d = QDate::fromString(dateStr.trimmed(), "yyyy-MM-dd");
        if (d.isValid()) dateStr = d.toString("yyyy-MM-dd");
    }

    QString err;
    if (!DbManager::instance().addLog(m_userId, title, person, place, dateStr, score, &err)) {
        QMessageBox::warning(this, "添加失败", err);
        return;
    }

    refreshData();
}

void LogRecordWindow::onRefreshClicked()
{
    ui->log_search_content_lineEdit->clear();
    m_proxy->setFilterRegularExpression(QRegularExpression());
    refreshData();
}

void LogRecordWindow::onSearchClicked()
{
    const QString kw = ui->log_search_content_lineEdit->text().trimmed();
    if (kw.isEmpty()) {
        m_proxy->setFilterRegularExpression(QRegularExpression());
    } else {
        // 直接用 pattern 存关键字，LogFilterProxyModel 会按 contains 来匹配
        m_proxy->setFilterRegularExpression(QRegularExpression(kw, QRegularExpression::CaseInsensitiveOption));
    }
}

void LogRecordWindow::onExportCsvClicked()
{
    if (!m_proxy) return;

    const int rows = m_proxy->rowCount();
    if (rows <= 0) {
        QMessageBox::information(this, "导出CSV", "当前没有可导出的记录。");
        return;
    }

    const QString defaultName = QString("logs_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString defaultPath = QDir::home().absoluteFilePath(defaultName);

    QString filePath = QFileDialog::getSaveFileName(this, "导出事件记录为 CSV", defaultPath, "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(".csv", Qt::CaseInsensitive)) filePath += ".csv";

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "导出失败", QString("无法写入文件：\n%1").arg(filePath));
        return;
    }

    QTextStream out(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#endif
    // 写 BOM，确保 Excel 打开中文不乱码
    out << QChar(0xFEFF);

    // 只导出当前 tableView 可见的列
    QVector<int> cols;
    cols.reserve(m_proxy->columnCount());
    for (int c = 0; c < m_proxy->columnCount(); ++c) {
        if (!ui->tableView->isColumnHidden(c)) cols.push_back(c);
    }
    if (cols.isEmpty()) {
        QMessageBox::warning(this, "导出失败", "没有可导出的列。");
        return;
    }

    auto csvEscape = [](QString s) -> QString {
        s.replace("\"", "\"\"");
        if (s.contains(',') || s.contains('"') || s.contains('\n') || s.contains('\r')) {
            s = "\"" + s + "\"";
        }
        return s;
    };

    // 表头
    {
        QStringList header;
        header.reserve(cols.size());
        for (int c : cols) {
            header << csvEscape(m_proxy->headerData(c, Qt::Horizontal).toString());
        }
        out << header.join(",") << "\n";
    }

    // 数据行
    for (int r = 0; r < rows; ++r) {
        QStringList line;
        line.reserve(cols.size());
        for (int c : cols) {
            const QString v = m_proxy->data(m_proxy->index(r, c), Qt::DisplayRole).toString();
            line << csvEscape(v);
        }
        out << line.join(",") << "\n";
    }

    f.close();
    QMessageBox::information(this, "导出完成",
                             QString("已导出 %1 条记录到：\n%2").arg(rows).arg(filePath));
}

void LogRecordWindow::onTableContextMenuRequested(const QPoint& pos)
{
    const QModelIndex proxyIdx = ui->tableView->indexAt(pos);
    if (!proxyIdx.isValid()) return;

    QMenu menu(this);
    QAction* actDelete = menu.addAction("删除选中记录");
    QAction* chosen = menu.exec(ui->tableView->viewport()->mapToGlobal(pos));
    if (chosen != actDelete) return;

    const QModelIndex srcIdx = m_proxy->mapToSource(proxyIdx);
    const int row = srcIdx.row();
    const int logId = m_model->data(m_model->index(row, 0)).toInt();

    const auto ret = QMessageBox::question(this, "确认删除", "确定删除该记录吗？");
    if (ret != QMessageBox::Yes) return;

    QString err;
    if (!DbManager::instance().deleteLogById(logId, &err)) {
        QMessageBox::warning(this, "删除失败", err);
        return;
    }
    refreshData();
}
