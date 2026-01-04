#include "loginwindow.h"
#include "ui_login_window.h"

#include <QMessageBox>
#include <QPushButton>

#include "registerwindow.h"
#include "mainwindow.h"
#include "dbmanager.h"

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);

    ui->lineEditPass->setEchoMode(QLineEdit::Password);

    connect(ui->login_pushButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(ui->btnRegister, &QPushButton::clicked, this, &LoginWindow::openRegister);

    bindOtherLoginButtonsByText();
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::bindOtherLoginButtonsByText()
{
    // 你没给我“其他登录方式”按钮的 objectName，所以用按钮文本匹配绑定
    const auto buttons = this->findChildren<QPushButton*>();
    for (auto *b : buttons) {
        if (!b) continue;
        const QString t = b->text().trimmed();
        if (t.contains("其他登录方式") || t.contains("其他登录") || t.contains("第三方")) {
            connect(b, &QPushButton::clicked, this, &LoginWindow::otherLoginNotReady);
        }
    }
}

void LoginWindow::otherLoginNotReady()
{
    QMessageBox::information(this, "提示", "此功能暂未完成。");
}

void LoginWindow::openRegister()
{
    if (!m_register) {
        m_register = new RegisterWindow();
        m_register->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(m_register, &RegisterWindow::backToLoginRequested, this, &LoginWindow::backFromRegister);
        connect(m_register, &QObject::destroyed, this, [this](){ m_register = nullptr; });
    }
    m_register->show();
    this->hide();
}

void LoginWindow::backFromRegister()
{
    this->show();
    if (m_register) m_register->close();
}

void LoginWindow::handleLogin()
{
    const QString user = ui->lineEditUser->text().trimmed();
    const QString pass = ui->lineEditPass->text();

    if (user.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, "登录", "请输入用户名和密码。");
        return;
    }

    if (!DbManager::instance().init()) {
        QMessageBox::critical(this, "登录", "数据库初始化失败。");
        return;
    }

    if (!DbManager::instance().loginUser(user, pass)) {
        QMessageBox::warning(this, "登录", "用户名或密码错误。");
        return;
    }

    // 登录成功 -> 进入主界面
    m_main = new MainWindow(user);
    m_main->show();
    close();
}
