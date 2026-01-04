#include "registerwindow.h"
#include "ui_register_window.h"

#include <QMessageBox>
#include "dbmanager.h"

RegisterWindow::RegisterWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RegisterWindow)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
}

RegisterWindow::~RegisterWindow()
{
    delete ui;
}

void RegisterWindow::initUi()
{
    setWindowTitle("Trip Memory · 旅忆 - 注册");
    ui->lineEditPass->setEchoMode(QLineEdit::Password);
    ui->lineEditConfirm->setEchoMode(QLineEdit::Password);
}

void RegisterWindow::connectSignals()
{
    connect(ui->register_pushButton, &QPushButton::clicked, this, &RegisterWindow::onRegisterClicked);
    connect(ui->return_pushButton, &QPushButton::clicked, this, &RegisterWindow::onReturnClicked);

    connect(ui->min_button, &QPushButton::clicked, this, &RegisterWindow::onMinClicked);
    connect(ui->max_button, &QPushButton::clicked, this, &RegisterWindow::onMaxClicked);
    connect(ui->close_button, &QPushButton::clicked, this, &RegisterWindow::onCloseClicked);
}

void RegisterWindow::onRegisterClicked()
{
    if (!DbManager::instance().isOpen()) {
        QMessageBox::critical(this, "错误", "数据库打开失败，请重启程序。");
        return;
    }

    const QString username = ui->lineEditUser->text().trimmed();
    const QString pass = ui->lineEditPass->text();
    const QString confirm = ui->lineEditConfirm->text();

    if (username.isEmpty() || pass.isEmpty() || confirm.isEmpty()) {
        QMessageBox::warning(this, "提示", "请完整填写注册信息。");
        return;
    }
    if (pass != confirm) {
        QMessageBox::warning(this, "提示", "两次输入的密码不一致。");
        return;
    }

    QString err;
    if (!DbManager::instance().registerUser(username, pass, &err)) {
        QMessageBox::warning(this, "注册失败", err.isEmpty() ? "注册失败，请更换用户名后重试。" : err);
        return;
    }

    emit registerSuccess(username);
}

void RegisterWindow::onReturnClicked()
{
    emit requestBack();
}

void RegisterWindow::onMinClicked()
{
    showMinimized();
}

void RegisterWindow::onMaxClicked()
{
    isMaximized() ? showNormal() : showMaximized();
}

void RegisterWindow::onCloseClicked()
{
    close();
}
