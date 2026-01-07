#include "messageadminwindow.h"
#include "ui_message_info_window.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDateTime>

#include "dbmanager.h"
#include "replywindow.h"

namespace {
static QString safeStr(const QVariant& v) { return v.isNull() ? "" : v.toString(); }
}

MessageAdminWindow::MessageAdminWindow(const QString& adminName, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MessageAdminWindow)
    , m_adminName(adminName)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
    refreshData(false);
}

MessageAdminWindow::~MessageAdminWindow()
{
    delete ui;
}

void MessageAdminWindow::initUi()
{
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MessageAdminWindow::connectSignals()
{
    connect(ui->refresh_pushButton, &QPushButton::clicked, this, &MessageAdminWindow::onRefreshClicked);
    connect(ui->search_sender_pushButton, &QPushButton::clicked, this, &MessageAdminWindow::onSearchClicked);
    connect(ui->noreply_pushButton, &QPushButton::clicked, this, &MessageAdminWindow::onNoReplyClicked);
    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &MessageAdminWindow::onCellDoubleClicked);
}

void MessageAdminWindow::refreshData(bool onlyNoReply)
{
    const QString senderFilter = ui->sender_search_lineEdit->text().trimmed();
    loadData(senderFilter, onlyNoReply);
}

void MessageAdminWindow::loadData(const QString& senderFilter, bool onlyNoReply)
{
    ui->tableWidget->setRowCount(0);

    QString sql =
        "SELECT id,sender,receiver,message,replied,reply,send_time,reply_time "
        "FROM messages WHERE receiver='admin' ";

    if (!senderFilter.isEmpty()) {
        sql += "AND sender LIKE ? ";
    }
    if (onlyNoReply) {
        sql += "AND replied=0 ";
    }
    sql += "ORDER BY id DESC";

    QSqlQuery q(DbManager::instance().database());
    q.prepare(sql);
    if (!senderFilter.isEmpty()) {
        q.addBindValue("%" + senderFilter + "%");
    }

    if (!q.exec()) {
        QMessageBox::warning(this, "查询失败", q.lastError().text());
        return;
    }

    int row=0;
    while (q.next()) {
        ui->tableWidget->insertRow(row);

        const int msgId = q.value(0).toInt();
        const QString sender = safeStr(q.value(1));
        const QString receiver = safeStr(q.value(2));
        const QString message = safeStr(q.value(3));
        const int replied = q.value(4).toInt();
        const QString reply = safeStr(q.value(5));
        const QString sendTime = safeStr(q.value(6));
        const QString replyTime = safeStr(q.value(7));

        // 0 发送者：保存 msgId
        auto* itemSender = new QTableWidgetItem(sender);
        itemSender->setData(Qt::UserRole, msgId);
        ui->tableWidget->setItem(row, 0, itemSender);

        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(receiver));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(message));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(replied ? "是" : "否"));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(reply));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(sendTime));
        ui->tableWidget->setItem(row, 6, new QTableWidgetItem(replyTime));

        row++;
    }

    ui->message_total_label->setText(QString("消息总数：%1").arg(row));
}

void MessageAdminWindow::onRefreshClicked()
{
    refreshData(false);
}

void MessageAdminWindow::onSearchClicked()
{
    refreshData(false);
}

void MessageAdminWindow::onNoReplyClicked()
{
    refreshData(true);
}

void MessageAdminWindow::onCellDoubleClicked(int row, int /*column*/)
{
    QTableWidgetItem* senderItem = ui->tableWidget->item(row, 0);
    if (!senderItem) return;

    const int msgId = senderItem->data(Qt::UserRole).toInt();
    const QString repliedText = ui->tableWidget->item(row, 3) ? ui->tableWidget->item(row, 3)->text() : "否";
    const bool alreadyReplied = (repliedText == "是");

    if (alreadyReplied) {
        QMessageBox::information(this, "已回复", "该消息已回复。\n你可以在表格中查看回复内容。");
        return;
    }

    ReplyWindow dlg(this);
    dlg.setHint("请输入回复内容（双击未回复消息可弹出此窗口）");
    if (dlg.exec() != QDialog::Accepted) return;

    QString err;
    if (!DbManager::instance().replyMessageById(msgId, dlg.getReplyText(), &err)) {
        QMessageBox::warning(this, "回复失败", err);
        return;
    }

    refreshData(false);
}
