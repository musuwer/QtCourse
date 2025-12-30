#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QHostAddress>
#include <QJsonValue>
#include <QJsonObject>
#include <QMessageBox> // 新增：用于弹窗提示

// 定义常量键名，防止手误拼写错误
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

    // 连接信号槽
    connect(m_chatClient, &ChatClient::connected, this, &MainWindow::connectedToServer);
    connect(m_chatClient, &ChatClient::jsonReceived, this, &MainWindow::jsonReceived);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnLogin_clicked()
{
    // 优化：增加输入校验
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
    // 优化：实现退出功能
    this->close();
}

void MainWindow::on_btnSent_clicked()
{
    QString text = ui->messageEdit->text();
    if (!text.isEmpty()) {
        m_chatClient->sendMessage(text);

        // 优化：发送后清空输入框，并让光标回到输入框，方便连续输入
        ui->messageEdit->clear();
        ui->messageEdit->setFocus();
    }
}

void MainWindow::on_btnLogout_clicked()
{
    // 切换界面
    ui->stackedWidget->setCurrentWidget(ui->loginPage);

    // 断开连接
    m_chatClient->disconnectFromHost();

    // 优化：注销意味着断开连接，直接清空整个用户列表即可
    // 原代码试图只删除自己，但在断开连接的上下文中，清空列表更合理
    ui->UserListWidget->clear();
}

void MainWindow::connectedToServer()
{
    ui->stackedWidget->setCurrentWidget(ui->chatPage);
    // 发送登录消息
    m_chatClient->sendMessage(ui->nameEdit->text(), "login");
}

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

    // 优化：使用 else if 结构，提高效率（匹配成功后不再判断后续条件）
    if (type.compare("message", Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = docObj.value(KEY_TEXT);
        const QJsonValue senderVal = docObj.value(KEY_SENDER);

        if (!textVal.isString() || !senderVal.isString()) return;

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
    ui->UserListWidget->addItem(user);
}

void MainWindow::userDelete(const QString &user)
{
    // 优化：修复了列表项删除逻辑
    // findItems 返回所有匹配的项
    QList<QListWidgetItem*> items = ui->UserListWidget->findItems(user, Qt::MatchExactly);
    for (auto item : items) {
        // removeItemWidget(item) 是错误的，它用于移除 setItemWidget 设置的控件
        // delete item 会自动将其从 QListWidget 中移除
        delete item;
    }
}

void MainWindow::userListReceived(const QStringList &list)
{
    ui->UserListWidget->clear();
    ui->UserListWidget->addItems(list);
}
