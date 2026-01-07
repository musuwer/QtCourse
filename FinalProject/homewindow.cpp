#include "homewindow.h"
#include "ui_home_window.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QMenu>
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

#include "dbmanager.h"
#include "goalrecordwindow.h"
#include "annoucewindow.h"

namespace {
static QString safeStr(const QVariant& v) { return v.isNull() ? "" : v.toString(); }
}

HomeWindow::HomeWindow(int userId, const QString& username, const QString& role, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HomeWindow)
    , m_userId(userId)
    , m_username(username)
    , m_role(role)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
    refreshAll();
}

HomeWindow::~HomeWindow()
{
    delete ui;
}

void HomeWindow::initUi()
{
    setupTableWidgets();

    if (m_role != "admin") {
        ui->add_annou_pushButton->setEnabled(false);
        ui->add_annou_pushButton->setToolTip("仅管理员可发布公告");
    }
}

void HomeWindow::setupTableWidgets()
{
    // goals
    ui->goal_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->goal_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->goal_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->goal_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->goal_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->goal_tableWidget->horizontalHeader()->setStretchLastSection(true);

    // announcements
    ui->annou_info_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->annou_info_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->annou_info_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->annou_info_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->annou_info_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->annou_info_tableWidget->horizontalHeader()->setStretchLastSection(true);
}

void HomeWindow::connectSignals()
{
    connect(ui->add_goal_pushButton, &QPushButton::clicked, this, &HomeWindow::onAddGoalClicked);
    connect(ui->add_annou_pushButton, &QPushButton::clicked, this, &HomeWindow::onAddAnnouncementClicked);

    connect(ui->refresh_goal_pushButton, &QPushButton::clicked, this, &HomeWindow::onRefreshGoalClicked);
    connect(ui->refresh_pushButton, &QPushButton::clicked, this, &HomeWindow::onRefreshAnnClicked);

    connect(ui->goal_tableWidget, &QTableWidget::customContextMenuRequested, this, &HomeWindow::onGoalContextMenu);
    connect(ui->annou_info_tableWidget, &QTableWidget::customContextMenuRequested, this, &HomeWindow::onAnnContextMenu);

    // ✅ 导出按钮由 .ui 提供
    if (ui->export_goal_csv_pushButton) {
        connect(ui->export_goal_csv_pushButton, &QPushButton::clicked, this, &HomeWindow::onExportGoalCsvClicked);
    }
}

void HomeWindow::refreshAll()
{
    refreshGoals();
    refreshAnnouncements();
}

void HomeWindow::refreshGoals()
{
    ui->goal_tableWidget->setRowCount(0);

    QSqlQuery q(DbManager::instance().database());
    if (m_role == "admin") {
        q.prepare("SELECT id,user_id,goal,plan,progress,start_time,end_time FROM goals ORDER BY id DESC");
    } else {
        q.prepare("SELECT id,user_id,goal,plan,progress,start_time,end_time FROM goals WHERE user_id=? ORDER BY id DESC");
        q.addBindValue(m_userId);
    }

    if (!q.exec()) {
        QMessageBox::warning(this, "查询失败", q.lastError().text());
        return;
    }

    int row = 0;
    while (q.next()) {
        ui->goal_tableWidget->insertRow(row);

        const int goalId = q.value(0).toInt();

        // 列 0：用户ID
        auto* item0 = new QTableWidgetItem(q.value(1).toString());
        item0->setData(Qt::UserRole, goalId); // 保存主键，方便删除
        ui->goal_tableWidget->setItem(row, 0, item0);

        ui->goal_tableWidget->setItem(row, 1, new QTableWidgetItem(safeStr(q.value(2))));
        ui->goal_tableWidget->setItem(row, 2, new QTableWidgetItem(safeStr(q.value(3))));
        ui->goal_tableWidget->setItem(row, 3, new QTableWidgetItem(safeStr(q.value(4))));
        ui->goal_tableWidget->setItem(row, 4, new QTableWidgetItem(safeStr(q.value(5))));
        ui->goal_tableWidget->setItem(row, 5, new QTableWidgetItem(safeStr(q.value(6))));

        row++;
    }

    ui->totol_goal_label->setText(QString("目标总数：%1").arg(row));
}

void HomeWindow::refreshAnnouncements()
{
    ui->annou_info_tableWidget->setRowCount(0);

    QSqlQuery q(DbManager::instance().database());
    if (!q.exec("SELECT id,title,content,created_at FROM announcements ORDER BY id DESC")) {
        QMessageBox::warning(this, "查询失败", q.lastError().text());
        return;
    }

    int row = 0;
    while (q.next()) {
        ui->annou_info_tableWidget->insertRow(row);

        const int annId = q.value(0).toInt();

        auto* item0 = new QTableWidgetItem(safeStr(q.value(1)));
        item0->setData(Qt::UserRole, annId);

        ui->annou_info_tableWidget->setItem(row, 0, item0);
        ui->annou_info_tableWidget->setItem(row, 1, new QTableWidgetItem(safeStr(q.value(2))));
        ui->annou_info_tableWidget->setItem(row, 2, new QTableWidgetItem(safeStr(q.value(3))));

        row++;
    }

    ui->totol_annouce_label->setText(QString("公告总数：%1").arg(row));
}

void HomeWindow::onAddGoalClicked()
{
    GoalRecordWindow dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    const QString goalText = dlg.getGoalText();
    const QString plan = "计划：每日打卡";
    const QString progress = "0%";
    const QString start = QDate::currentDate().toString("yyyy-MM-dd");
    const QString end = QDate::currentDate().addDays(7).toString("yyyy-MM-dd");

    QString err;
    if (!DbManager::instance().addGoal(m_userId, goalText, plan, progress, start, end, &err)) {
        QMessageBox::warning(this, "添加失败", err);
        return;
    }
    refreshGoals();
}

void HomeWindow::onAddAnnouncementClicked()
{
    if (m_role != "admin") {
        QMessageBox::information(this, "提示", "仅管理员可发布公告。");
        return;
    }

    AnnouceWindow dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    QString err;
    if (!DbManager::instance().addAnnouncement(m_username, dlg.getTitle(), dlg.getContent(), &err)) {
        QMessageBox::warning(this, "发布失败", err);
        return;
    }
    refreshAnnouncements();
}

void HomeWindow::onRefreshGoalClicked()
{
    refreshGoals();
}

void HomeWindow::onRefreshAnnClicked()
{
    refreshAnnouncements();
}

void HomeWindow::onExportGoalCsvClicked()
{
    if (!ui->goal_tableWidget) return;
    if (ui->goal_tableWidget->rowCount() <= 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    const QString defaultName = QString("goals_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString defaultPath = QDir::home().filePath(defaultName);
    QString filePath = QFileDialog::getSaveFileName(this, "导出目标记录为 CSV", defaultPath, "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(".csv", Qt::CaseInsensitive)) filePath += ".csv";

    // 导出可见列
    QVector<int> cols;
    cols.reserve(ui->goal_tableWidget->columnCount());
    for (int c = 0; c < ui->goal_tableWidget->columnCount(); ++c) {
        if (!ui->goal_tableWidget->isColumnHidden(c)) cols.push_back(c);
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
            auto* hi = ui->goal_tableWidget->horizontalHeaderItem(c);
            header << csvEscape(hi ? hi->text() : QString("col_%1").arg(c));
        }
        out << header.join(",") << "\n";
    }

    // 数据（跳过隐藏行）
    int exported = 0;
    for (int r = 0; r < ui->goal_tableWidget->rowCount(); ++r) {
        if (ui->goal_tableWidget->isRowHidden(r)) continue;
        QStringList row;
        row.reserve(cols.size());
        for (int c : cols) {
            const QTableWidgetItem* it = ui->goal_tableWidget->item(r, c);
            row << csvEscape(it ? it->text() : QString());
        }
        out << row.join(",") << "\n";
        exported++;
    }

    file.close();
    QMessageBox::information(this, "导出完成",
                             QString("已导出 %1 条目标记录到：\n%2").arg(exported).arg(QDir::toNativeSeparators(filePath)));
}

void HomeWindow::onGoalContextMenu(const QPoint& pos)
{
    const QModelIndex idx = ui->goal_tableWidget->indexAt(pos);
    if (!idx.isValid()) return;

    QMenu menu(this);
    QAction* actDelete = menu.addAction("删除目标");
    QAction* chosen = menu.exec(ui->goal_tableWidget->viewport()->mapToGlobal(pos));
    if (chosen != actDelete) return;

    const int row = idx.row();
    QTableWidgetItem* item0 = ui->goal_tableWidget->item(row, 0);
    if (!item0) return;

    const int goalId = item0->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this, "确认删除", "确定删除该目标吗？") != QMessageBox::Yes) return;

    QString err;
    if (!DbManager::instance().deleteGoalById(goalId, &err)) {
        QMessageBox::warning(this, "删除失败", err);
        return;
    }
    refreshGoals();
}

void HomeWindow::onAnnContextMenu(const QPoint& pos)
{
    if (m_role != "admin") return;

    const QModelIndex idx = ui->annou_info_tableWidget->indexAt(pos);
    if (!idx.isValid()) return;

    QMenu menu(this);
    QAction* actDelete = menu.addAction("删除公告");
    QAction* chosen = menu.exec(ui->annou_info_tableWidget->viewport()->mapToGlobal(pos));
    if (chosen != actDelete) return;

    const int row = idx.row();
    QTableWidgetItem* item0 = ui->annou_info_tableWidget->item(row, 0);
    if (!item0) return;

    const int annId = item0->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this, "确认删除", "确定删除该公告吗？") != QMessageBox::Yes) return;

    QString err;
    if (!DbManager::instance().deleteAnnouncementById(annId, &err)) {
        QMessageBox::warning(this, "删除失败", err);
        return;
    }
    refreshAnnouncements();
}
