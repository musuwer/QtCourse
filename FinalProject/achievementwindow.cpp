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
    , m_isAdmin(role == "admin")
{
    ui->setupUi(this);
    initUi();
    connectSignals();
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

    // 让表格列填满可用宽度
    if (ui->tableWidget->horizontalHeader()) {
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
}

void AchievementWindow::connectSignals()
{
    connect(ui->add_achieve_pushButton, &QPushButton::clicked, this, &AchievementWindow::onAddClicked);
    connect(ui->refresh_pushButton, &QPushButton::clicked, this, &AchievementWindow::onRefreshClicked);
    connect(ui->search_pushButton, &QPushButton::clicked, this, &AchievementWindow::onSearchClicked);
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested, this, &AchievementWindow::onTableContextMenu);
}

void AchievementWindow::refreshData()
{
    ui->tableWidget->setRowCount(0);

    QSqlQuery q(DbManager::instance().database());
    if (m_isAdmin) {
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
    QString sql;
    if (m_isAdmin) {
        sql = QString("SELECT id,user_id,name,type,level,org,ach_date,description FROM achievements WHERE %1 LIKE ? ORDER BY id DESC").arg(col);
        q.prepare(sql);
        q.addBindValue("%" + kw + "%");
    } else {
        sql = QString("SELECT id,user_id,name,type,level,org,ach_date,description FROM achievements WHERE user_id=? AND %1 LIKE ? ORDER BY id DESC").arg(col);
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
