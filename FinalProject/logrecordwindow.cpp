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
}

void LogRecordWindow::updateCountLabel()
{
    if (!m_proxy) return;
    ui->book_total_label->setText(QString("总计：%1").arg(m_proxy->rowCount()));
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
