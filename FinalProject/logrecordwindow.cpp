#include "logrecordwindow.h"
#include "ui_logrecord_window.h"
#include "dbmanager.h"

LogRecordWindow::LogRecordWindow(int userId, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogRecordWindow),
    m_currentUserId(userId)
{
    ui->setupUi(this);

    // --- 核心 Model/View 实现 ---
    m_model = new QSqlTableModel(this);
    m_model->setTable("logs");
    // 设置过滤器，只显示当前用户的日志
    m_model->setFilter(QString("user_id = %1").arg(m_currentUserId));
    m_model->select(); // 查询数据

    // 设置表头别名
    m_model->setHeaderData(2, Qt::Horizontal, tr("Date"));
    m_model->setHeaderData(3, Qt::Horizontal, tr("Mood"));
    m_model->setHeaderData(4, Qt::Horizontal, tr("Content"));

    // 将 Model 绑定到 View (假设 UI 里有个 QTableView 叫 tableView)
    // 如果你的 UI 里是 QTableWidget，请在 Designer 里把它删了换成 QTableView！
    ui->tableView->setModel(m_model);

    // 隐藏 ID 列和 UserID 列
    ui->tableView->setColumnHidden(0, true);
    ui->tableView->setColumnHidden(1, true);

    // 优化显示
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止直接编辑，或允许编辑
}

LogRecordWindow::~LogRecordWindow()
{
    delete ui;
}

void LogRecordWindow::refreshData() {
    m_model->select(); // 重新查询数据库刷新界面
}

void LogRecordWindow::on_btnAddLog_clicked() {
    // 这里弹出 AddLogWindow
    // AddLogWindow 保存数据到数据库后，调用 this->refreshData();
}

void LogRecordWindow::on_btnDeleteLog_clicked() {
    // 获取当前选中的行
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(selection.count() > 0) {
        m_model->removeRow(selection.at(0).row());
        m_model->submitAll(); // 提交删除到数据库
    }
}
