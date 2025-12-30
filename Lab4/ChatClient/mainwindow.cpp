#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QHostAddress>
#include <QJsonValue>
#include <QJsonObject>
#include <QMessageBox> // 新增：用于弹窗提示错误

// 定义常量键名，防止手误拼写错误 (避免之前的 "type" vs "Type" 问题)
const QString KEY_TYPE = "type";
const QString KEY_TEXT = "text";
const QString KEY_SENDER = "sender";
const QString KEY_USERNAME = "userName";
const QString KEY_USERLIST = "userList";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->loginPage);

    m_chatClient = new ChatClient(this);

    connect(m_chatClient, &ChatClient::connected, this, &MainWindow::connectedToServer);
    connect(m_chatClient, &ChatClient::jsonReceived, this, &MainWindow::jsonReceived);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnLogin_clicked()
{
    // 优化1：增加输入校验，防止空IP或空名字导致连接错误
    QString ip = ui->IPEdit->text().trimmed();
    QString name = ui->nameEdit->text().trimmed();

    if (ip.isEmpty() || name.isEmpty()) {
        QMessageBox::warning(this, "提示", "IP地址和昵称不能为空！");
        return;
    }

    // 尝试连接
    m_chatClient->connectToServer(QHostAddress(ip), 1967);
}

void MainWindow::on_btnExit_clicked()
{
    // 优化2：实现退出按钮功能
    this->close();
}

void MainWindow::on_btnSent_clicked()
{
    QString text = ui->messageEdit->text();
    if (!text.isEmpty()) {
        m_chatClient->sendMessage(text);

        // 优化3：发送后清空输入框，并让光标自动回到输入框，方便连续打字
        ui->messageEdit->clear();
        ui->messageEdit->setFocus();
    }
}

void MainWindow::on_btnLogout_clicked()
{
    // 切换回登录界面
    ui->stackedWidget->setCurrentWidget(ui->loginPage);

    // 断开连接
    m_chatClient->disconnectFromHost();

    // 优化4：注销意味着断开连接，直接清空整个用户列表即可
    // 原代码只删除了自己，但实际上断开后看不到任何人了，清空最合理
    ui->UserListWidget->clear();
}

void MainWindow::connectedToServer()
{
    ui->stackedWidget->setCurrentWidget(ui->chatPage);
    // 发送登录消息
    m_chatClient->sendMessage(ui->nameEdit->text(), "login");
}

// 这个函数目前看起来没用到，因为逻辑都走 jsonReceived 了，可以保留作为备用或删除
void MainWindow::messageReviced(const QString &sender, const QString &text)
{
    ui->roomTextEdit->append(QString("%1 : %2").arg(sender).arg(text));
}

void MainWindow::jsonReceived(const QJsonObject &docObj)
{
    const QJsonValue typeVal = docObj.value(KEY_TYPE);
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }

    const QString type = typeVal.toString();

    // 优化5：使用 else if 结构，一旦匹配成功就不再判断后面的，提高效率
    if (type.compare("message", Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = docObj.value(KEY_TEXT);
        const QJsonValue senderVal = docObj.value(KEY_SENDER);

        if (!textVal.isString() || !senderVal.isString()) return;

        // 简单的显示格式优化
        QString msg = QString("%1 : %2").arg(senderVal.toString(), textVal.toString());
        ui->roomTextEdit->append(msg);
    }
    else if (type.compare("newUser", Qt::CaseInsensitive) == 0) {
        const QJsonValue nameVal = docObj.value(KEY_USERNAME);
        if (!nameVal.isString()) return;

        userJoined(nameVal.toString());
    }
    else if (type.compare("userDisconnected", Qt::CaseInsensitive) == 0) {
        const QJsonValue nameVal = docObj.value(KEY_USERNAME);
        if (!nameVal.isString()) return;

        userDelete(nameVal.toString());
    }
    else if (type.compare("userList", Qt::CaseInsensitive) == 0) {
        const QJsonValue userListVal = docObj.value(KEY_USERLIST);
        if (!userListVal.isArray()) return;

        userListReceived(userListVal.toVariant().toStringList());
    }
}

void MainWindow::userJoined(const QString &user)
{
    // 优化：避免重复添加同一个用户显示在列表中
    if (ui->UserListWidget->findItems(user, Qt::MatchExactly).isEmpty()) {
        ui->UserListWidget->addItem(user);
    }
}


void MainWindow::userDelete(const QString &user)
{
    for(auto aItem:ui->UserListWidget->findItems(user,Qt::MatchExactly)){
        qDebug()<<"auto aItem:ui->U";
        ui->UserListWidget->removeItemWidget(aItem);
        delete aItem;
    }
}

void MainWindow::userListReceived(const QStringList &list)
{
    ui->UserListWidget->clear();
    ui->UserListWidget->addItems(list);
}

