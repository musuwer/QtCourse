#include "messageuserwindow.h"
#include "ui_message_info_user_window.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

#include "dbmanager.h"

namespace {
static QString safeStr(const QVariant& v) { return v.isNull() ? "" : v.toString(); }
}

MessageUserWindow::MessageUserWindow(const QString& username, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MessageUserWindow)
    , m_username(username)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
    refreshData();
}

MessageUserWindow::~MessageUserWindow()
{
    delete ui;
}

void MessageUserWindow::initUi()
{
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MessageUserWindow::connectSignals()
{
    connect(ui->send_pushButton, &QPushButton::clicked, this, &MessageUserWindow::onSendClicked);
    connect(ui->refresh_pushButton, &QPushButton::clicked, this, &MessageUserWindow::onRefreshClicked);
}

void MessageUserWindow::refreshData()
{
    ui->tableWidget->setRowCount(0);

    QSqlQuery q(DbManager::instance().database());
    q.prepare(R"SQL(
        SELECT sender, message, send_time, reply, reply_time
        FROM messages
        WHERE sender=? AND receiver='admin'
        ORDER BY id DESC
    )SQL");
    q.addBindValue(m_username);

    if (!q.exec()) {
        QMessageBox::warning(this, "查询失败", q.lastError().text());
        return;
    }

    int row=0;
    while (q.next()) {
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(safeStr(q.value(0))));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(safeStr(q.value(1))));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(safeStr(q.value(2))));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(safeStr(q.value(3))));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(safeStr(q.value(4))));
        row++;
    }

    ui->message_total_label->setText(QString("消息总数：%1").arg(row));
}

void MessageUserWindow::onSendClicked()
{
    const QString msg = ui->message_lineEdit->text().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入消息内容。");
        return;
    }

    QString err;
    if (!DbManager::instance().addMessage(m_username, "admin", msg, &err)) {
        QMessageBox::warning(this, "发送失败", err);
        return;
    }

    ui->message_lineEdit->clear();
    refreshData();
}

void MessageUserWindow::onRefreshClicked()
{
    refreshData();
}
