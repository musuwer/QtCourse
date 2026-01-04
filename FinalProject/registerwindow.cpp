#include "registerwindow.h"
#include "ui_register_window.h"

#include <QMessageBox>
#include "dbmanager.h"

RegisterWindow::RegisterWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RegisterWindow)
{
    ui->setupUi(this);

    ui->lineEditPass->setEchoMode(QLineEdit::Password);
    ui->lineEditConfirm->setEchoMode(QLineEdit::Password);

    connect(ui->register_pushButton, &QPushButton::clicked, this, &RegisterWindow::handleRegister);
    connect(ui->return_pushButton, &QPushButton::clicked, this, &RegisterWindow::handleReturn);
}

RegisterWindow::~RegisterWindow()
{
    delete ui;
}

void RegisterWindow::handleReturn()
{
    emit backToLoginRequested();
    close();
}

void RegisterWindow::handleRegister()
{
    const QString user = ui->lineEditUser->text().trimmed();
    const QString pass = ui->lineEditPass->text();
    const QString confirm = ui->lineEditConfirm->text();

    if (user.isEmpty() || pass.isEmpty() || confirm.isEmpty()) {
        QMessageBox::warning(this, "注册", "请填写完整信息。");
        return;
    }
    if (pass != confirm) {
        QMessageBox::warning(this, "注册", "两次输入的密码不一致。");
        return;
    }

    if (!DbManager::instance().init()) {
        QMessageBox::critical(this, "注册", "数据库初始化失败。");
        return;
    }

    if (!DbManager::instance().registerUser(user, pass)) {
        QMessageBox::warning(this, "注册", "注册失败：用户名可能已存在。");
        return;
    }

    QMessageBox::information(this, "注册", "注册成功，请返回登录。");
    emit backToLoginRequested();
    close();
}
