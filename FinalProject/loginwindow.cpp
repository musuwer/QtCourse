#include "loginwindow.h"
#include "ui_login_window.h"

#include <QMessageBox>

#include "registerwindow.h"
#include "mainwindow.h"
#include "dbmanager.h"

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::initUi()
{
    setWindowTitle("Trip Memory · 旅忆 - 登录");

    // 安全起见：如果 UI 没设置 echoMode，这里设置一下
    ui->lineEditPass->setEchoMode(QLineEdit::Password);
}

void LoginWindow::connectSignals()
{
    connect(ui->login_pushButton, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(ui->btnRegister, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);

    connect(ui->min_button, &QPushButton::clicked, this, &LoginWindow::onMinClicked);
    connect(ui->max_button, &QPushButton::clicked, this, &LoginWindow::onMaxClicked);
    connect(ui->close_button, &QPushButton::clicked, this, &LoginWindow::onCloseClicked);
}

void LoginWindow::onLoginClicked()
{
    if (!DbManager::instance().isOpen()) {
        QMessageBox::critical(this, "错误", "数据库打开失败，请重启程序。");
        return;
    }

    const QString username = ui->lineEditUser->text().trimmed();
    const QString password = ui->lineEditPass->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户名和密码。");
        return;
    }

    int userId = -1;
    QString role;
    QString err;
    if (!DbManager::instance().loginUser(username, password, &userId, &role, &err)) {
        QMessageBox::warning(this, "登录失败", err.isEmpty() ? "账号或密码错误" : err);
        return;
    }

    // 打开主窗口
    if (m_mainWindow) {
        m_mainWindow->deleteLater();
        m_mainWindow = nullptr;
    }
    m_mainWindow = new MainWindow(username);
    connect(m_mainWindow, &MainWindow::logoutRequested, this, [this]() {
        if (m_mainWindow) m_mainWindow->deleteLater();
        m_mainWindow = nullptr;
        this->show();
    });
    m_mainWindow->show();
    this->hide();
}

void LoginWindow::onRegisterClicked()
{
    if (!m_registerWindow) {
        m_registerWindow = new RegisterWindow();
        connect(m_registerWindow, &RegisterWindow::requestBack, this, &LoginWindow::onRegisterBack);
        connect(m_registerWindow, &RegisterWindow::registerSuccess, this, &LoginWindow::onRegisterSuccess);
    }
    m_registerWindow->show();
    this->hide();
}

void LoginWindow::onMinClicked()
{
    showMinimized();
}

void LoginWindow::onMaxClicked()
{
    isMaximized() ? showNormal() : showMaximized();
}

void LoginWindow::onCloseClicked()
{
    close();
}

void LoginWindow::onRegisterBack()
{
    if (m_registerWindow) m_registerWindow->hide();
    this->show();
}

void LoginWindow::onRegisterSuccess(const QString& username)
{
    // 注册成功后回到登录页，并自动填入用户名
    if (m_registerWindow) m_registerWindow->hide();
    this->show();
    ui->lineEditUser->setText(username);
    ui->lineEditPass->clear();
    QMessageBox::information(this, "注册成功", "注册成功，请登录。");
}
