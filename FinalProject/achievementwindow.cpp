#include "achievementwindow.h"
#include "ui_achievement_window.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QMenu>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QDate>
#include <QHeaderView>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStringConverter>
#include <QDir>
#include <QVector>

// QtCharts（Qt6 下常用方式：直接使用 QChart/QChartView 等全局类名）
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

#include <QVBoxLayout>
#include <QMap>
#include <algorithm>

#include "dbmanager.h"

namespace {
static QString safeStr(const QVariant& v) { return v.isNull() ? "" : v.toString(); }

static QString columnNameFromComboIndex(int idx)
{
    // comboBox items: 用户ID/成就名称/成就类型/成就级别/颁奖机构/获得时间/描述说明
    switch (idx) {
    case 0: return "user_id";
    case 1: return "name";
    case 2: return "type";
    case 3: return "level";
    case 4: return "org";
    case 5: return "ach_date";
    case 6: return "description";
    default: return "name";
    }
}
}

AchievementWindow::AchievementWindow(int userId, const QString& username, const QString& role, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AchievementWindow)
    , m_userId(userId)
    , m_username(username)
    , m_role(role)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
    initChartsIfNeeded();
    refreshData();
}

AchievementWindow::~AchievementWindow()
{
    delete ui;
}

void AchievementWindow::initUi()
{
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
}

void AchievementWindow::connectSignals()
{
    connect(ui->add_achieve_pushButton, &QPushButton::clicked, this, &AchievementWindow::onAddClicked);
    connect(ui->refresh_pushButton, &QPushButton::clicked, this, &AchievementWindow::onRefreshClicked);
    connect(ui->search_pushButton, &QPushButton::clicked, this, &AchievementWindow::onSearchClicked);
    // ✅ 导出按钮由 .ui 提供
    if (ui->export_csv_pushButton) {
        connect(ui->export_csv_pushButton, &QPushButton::clicked, this, &AchievementWindow::onExportCsvClicked);
    }
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested, this, &AchievementWindow::onTableContextMenu);
}

void AchievementWindow::onExportCsvClicked()
{
    if (!ui->tableWidget) return;
    if (ui->tableWidget->rowCount() <= 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    const QString defaultName = QString("achievements_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString defaultPath = QDir::home().filePath(defaultName);
    QString filePath = QFileDialog::getSaveFileName(this, "导出成就记录为 CSV", defaultPath, "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(".csv", Qt::CaseInsensitive)) filePath += ".csv";

    // 导出可见列
    QVector<int> cols;
    cols.reserve(ui->tableWidget->columnCount());
    for (int c = 0; c < ui->tableWidget->columnCount(); ++c) {
        if (!ui->tableWidget->isColumnHidden(c)) cols.push_back(c);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法写入文件：" + file.errorString());
        return;
    }

    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    out << QChar(0xFEFF);

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
            auto* hi = ui->tableWidget->horizontalHeaderItem(c);
            header << csvEscape(hi ? hi->text() : QString("col_%1").arg(c));
        }
        out << header.join(",") << "\n";
    }

    // 数据（跳过隐藏行）
    int exported = 0;
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        if (ui->tableWidget->isRowHidden(r)) continue;
        QStringList row;
        row.reserve(cols.size());
        for (int c : cols) {
            const QTableWidgetItem* it = ui->tableWidget->item(r, c);
            row << csvEscape(it ? it->text() : QString());
        }
        out << row.join(",") << "\n";
        exported++;
    }

    file.close();
    QMessageBox::information(this, "导出完成",
                             QString("已导出 %1 条成就记录到：\n%2").arg(exported).arg(QDir::toNativeSeparators(filePath)));
}

void AchievementWindow::refreshData()
{
    ui->tableWidget->setRowCount(0);

    QSqlQuery q(DbManager::instance().database());
    if (m_role == "admin") {
        q.prepare("SELECT id,user_id,name,type,level,org,ach_date,description FROM achievements ORDER BY id DESC");
    } else {
        q.prepare("SELECT id,user_id,name,type,level,org,ach_date,description FROM achievements WHERE user_id=? ORDER BY id DESC");
        q.addBindValue(m_userId);
    }

    if (!q.exec()) {
        QMessageBox::warning(this, "查询失败", q.lastError().text());
        return;
    }

    int row = 0;
    while (q.next()) {
        ui->tableWidget->insertRow(row);

        const int achId = q.value(0).toInt();
        auto* itemUserId = new QTableWidgetItem(q.value(1).toString());
        itemUserId->setData(Qt::UserRole, achId);

        ui->tableWidget->setItem(row, 0, itemUserId);
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(safeStr(q.value(2))));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(safeStr(q.value(3))));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(safeStr(q.value(4))));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(safeStr(q.value(5))));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(safeStr(q.value(6))));
        ui->tableWidget->setItem(row, 6, new QTableWidgetItem(safeStr(q.value(7))));

        row++;
    }

    // 让图表跟随数据刷新
    updateChartsFromTable();
}

void AchievementWindow::doSearch()
{
    const QString kw = ui->search_lineEdit->text().trimmed();
    if (kw.isEmpty()) {
        refreshData();
        return;
    }

    ui->tableWidget->setRowCount(0);

    const QString col = columnNameFromComboIndex(ui->comboBox->currentIndex());

    QSqlQuery q(DbManager::instance().database());
    if (m_role == "admin") {
        const QString sql = QString("SELECT id,user_id,name,type,level,org,ach_date,description FROM achievements WHERE %1 LIKE ? ORDER BY id DESC").arg(col);
        q.prepare(sql);
        q.addBindValue("%" + kw + "%");
    } else {
        const QString sql = QString("SELECT id,user_id,name,type,level,org,ach_date,description FROM achievements WHERE user_id=? AND %1 LIKE ? ORDER BY id DESC").arg(col);
        q.prepare(sql);
        q.addBindValue(m_userId);
        q.addBindValue("%" + kw + "%");
    }

    if (!q.exec()) {
        QMessageBox::warning(this, "搜索失败", q.lastError().text());
        return;
    }

    int row=0;
    while (q.next()) {
        ui->tableWidget->insertRow(row);

        const int achId = q.value(0).toInt();
        auto* itemUserId = new QTableWidgetItem(q.value(1).toString());
        itemUserId->setData(Qt::UserRole, achId);

        ui->tableWidget->setItem(row, 0, itemUserId);
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(safeStr(q.value(2))));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(safeStr(q.value(3))));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(safeStr(q.value(4))));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(safeStr(q.value(5))));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(safeStr(q.value(6))));
        ui->tableWidget->setItem(row, 6, new QTableWidgetItem(safeStr(q.value(7))));
        row++;
    }

    updateChartsFromTable();
}

void AchievementWindow::onAddClicked()
{
    // 基础版本：用一个表单弹窗收集字段
    QDialog dlg(this);
    dlg.setWindowTitle("新增成就");
    QFormLayout layout(&dlg);

    QLineEdit nameEd;
    QLineEdit typeEd;
    QLineEdit levelEd;
    QLineEdit orgEd;
    QLineEdit dateEd;
    QTextEdit descEd;

    dateEd.setPlaceholderText("yyyy-MM-dd（不填则用今天）");

    layout.addRow("成就名称：", &nameEd);
    layout.addRow("成就类型：", &typeEd);
    layout.addRow("成就级别：", &levelEd);
    layout.addRow("颁奖机构：", &orgEd);
    layout.addRow("获得时间：", &dateEd);
    layout.addRow("描述说明：", &descEd);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addRow(&buttons);

    QObject::connect(&buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    QString name = nameEd.text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "提示", "成就名称不能为空。");
        return;
    }

    QString dateStr = dateEd.text().trimmed();
    if (dateStr.isEmpty()) dateStr = QDate::currentDate().toString("yyyy-MM-dd");

    QString err;
    if (!DbManager::instance().addAchievement(
            m_userId, name,
            typeEd.text().trimmed(),
            levelEd.text().trimmed(),
            orgEd.text().trimmed(),
            dateStr,
            descEd.toPlainText().trimmed(),
            &err)) {
        QMessageBox::warning(this, "添加失败", err);
        return;
    }

    refreshData();
}

void AchievementWindow::onRefreshClicked()
{
    ui->search_lineEdit->clear();
    refreshData();
}

void AchievementWindow::onSearchClicked()
{
    doSearch();
}

void AchievementWindow::onTableContextMenu(const QPoint& pos)
{
    const QModelIndex idx = ui->tableWidget->indexAt(pos);
    if (!idx.isValid()) return;

    QMenu menu(this);
    QAction* actDelete = menu.addAction("删除成就");
    QAction* chosen = menu.exec(ui->tableWidget->viewport()->mapToGlobal(pos));
    if (chosen != actDelete) return;

    const int row = idx.row();
    QTableWidgetItem* item0 = ui->tableWidget->item(row, 0);
    if (!item0) return;

    const int achId = item0->data(Qt::UserRole).toInt();

    if (QMessageBox::question(this, "确认删除", "确定删除该成就吗？") != QMessageBox::Yes) return;

    QString err;
    if (!DbManager::instance().deleteAchievementById(achId, &err)) {
        QMessageBox::warning(this, "删除失败", err);
        return;
    }
    refreshData();
}

void AchievementWindow::initChartsIfNeeded()
{
    if (m_typeChartView && m_levelPieView) return;
    if (!ui) return;

    QWidget* leftHost = ui->bar_widget;
    QWidget* rightHost = ui->bar_widget_3;

    // 兼容：若 .ui 未包含这些占位控件，则尝试按 objectName 查找
    if (!leftHost) leftHost = findChild<QWidget*>(QStringLiteral("bar_widget"));
    if (!rightHost) rightHost = findChild<QWidget*>(QStringLiteral("bar_widget_3"));
    if (!leftHost || !rightHost) {
        // UI 没有预留区域，直接不创建图表（避免崩溃）
        return;
    }

    auto ensureLayout = [](QWidget* host) {
        if (!host->layout()) {
            auto* lay = new QVBoxLayout(host);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(0);
        }
    };

    ensureLayout(leftHost);
    ensureLayout(rightHost);

    // 左：柱状图（先创建空 Chart，后续 refreshData() 时填充数据）
    {
        auto* chart = new QChart();
        chart->setTitle(QStringLiteral("成就类型分布"));
        chart->setAnimationOptions(QChart::NoAnimation);

        m_typeChartView = new QChartView(chart, leftHost);
        m_typeChartView->setRenderHint(QPainter::Antialiasing, true);
        m_typeChartView->setRubberBand(QChartView::NoRubberBand);
        leftHost->layout()->addWidget(m_typeChartView);
    }

    // 右：扇形图
    {
        auto* chart = new QChart();
        chart->setTitle(QStringLiteral("成就级别占比"));
        chart->setAnimationOptions(QChart::NoAnimation);

        m_levelPieView = new QChartView(chart, rightHost);
        m_levelPieView->setRenderHint(QPainter::Antialiasing, true);
        m_levelPieView->setRubberBand(QChartView::NoRubberBand);
        rightHost->layout()->addWidget(m_levelPieView);
    }
}

void AchievementWindow::updateChartsFromTable()
{
    initChartsIfNeeded();
    if (!m_typeChartView || !m_levelPieView || !ui || !ui->tableWidget) return;

    QMap<QString, int> typeCount;
    QMap<QString, int> levelCount;

    const int rows = ui->tableWidget->rowCount();
    for (int r = 0; r < rows; ++r) {
        if (ui->tableWidget->isRowHidden(r)) continue;

        const auto* typeIt = ui->tableWidget->item(r, 2);
        const auto* levelIt = ui->tableWidget->item(r, 3);

        QString type = typeIt ? typeIt->text().trimmed() : QString();
        QString level = levelIt ? levelIt->text().trimmed() : QString();

        if (type.isEmpty()) type = QStringLiteral("未分类");
        if (level.isEmpty()) level = QStringLiteral("未知");

        typeCount[type] += 1;
        levelCount[level] += 1;
    }

    // -------- 左：柱状图（按类型） --------
    {
        auto* set = new QBarSet(QStringLiteral("数量"));
        QStringList categories;

        // 排序：按数量降序
        QList<QPair<QString,int>> items;
        for (auto it = typeCount.cbegin(); it != typeCount.cend(); ++it) {
            items.push_back({it.key(), it.value()});
        }
        std::sort(items.begin(), items.end(), [](const auto& a, const auto& b){
            if (a.second != b.second) return a.second > b.second;
            return a.first < b.first;
        });

        for (const auto& p : items) {
            categories << p.first;
            *set << p.second;
        }

        auto* series = new QBarSeries();
        series->append(set);

        // 复用 initChartsIfNeeded() 创建的 chart，避免 setChart 造成所有权/释放差异
        QChart* chart = m_typeChartView->chart();
        chart->removeAllSeries();
        for (auto* ax : chart->axes()) {
            chart->removeAxis(ax);
            ax->deleteLater();
        }
        chart->addSeries(series);
        chart->setTitle(QStringLiteral("成就类型分布"));
        chart->legend()->hide();
        chart->setAnimationOptions(QChart::SeriesAnimations);

        auto* axisX = new QBarCategoryAxis();
        axisX->append(categories);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        auto* axisY = new QValueAxis();
        axisY->setLabelFormat("%d");
        axisY->setMin(0);
        axisY->setMax(qMax(1, rows));
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    }

    // -------- 右：扇形图（按级别） --------
    {
        auto* series = new QPieSeries();

        QList<QPair<QString,int>> items;
        for (auto it = levelCount.cbegin(); it != levelCount.cend(); ++it) {
            items.push_back({it.key(), it.value()});
        }
        std::sort(items.begin(), items.end(), [](const auto& a, const auto& b){
            if (a.second != b.second) return a.second > b.second;
            return a.first < b.first;
        });

        int maxV = 0;
        for (const auto& p : items) {
            series->append(p.first, p.second);
            maxV = qMax(maxV, p.second);
        }
        series->setLabelsVisible(true);

        // 突出显示最大的一块
        for (auto* slice : series->slices()) {
            if (static_cast<int>(slice->value()) == maxV && maxV > 0) {
                slice->setExploded(true);
                slice->setLabelVisible(true);
                break;
            }
        }

        QChart* chart = m_levelPieView->chart();
        chart->removeAllSeries();
        for (auto* ax : chart->axes()) {
            chart->removeAxis(ax);
            ax->deleteLater();
        }
        chart->addSeries(series);
        chart->setTitle(QStringLiteral("成就级别占比"));
        chart->legend()->setAlignment(Qt::AlignBottom);
        chart->setAnimationOptions(QChart::SeriesAnimations);
    }
}
