#include "loginwindow.h"
#include "ui_login_window.h"

#include <QMessageBox>
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>
#include <QAbstractButton>

#include "registerwindow.h"
#include "mainwindow.h"
#include "dbmanager.h"

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);

    initFrameless();

    ui->lineEditPass->setEchoMode(QLineEdit::Password);

    connect(ui->login_pushButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(ui->btnRegister, &QPushButton::clicked, this, &LoginWindow::openRegister);

    bindOtherLoginButtonsByText();
}

void LoginWindow::initFrameless()
{
    // 去掉系统标题栏/边框，使用 ui 自带的按钮
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    // 顶部栏用于拖拽
    if (ui->frame_4) {
        ui->frame_4->installEventFilter(this);

        const auto widgets = ui->frame_4->findChildren<QWidget*>();
        for (QWidget* w : widgets) {
            if (!w) continue;
            if (qobject_cast<QAbstractButton*>(w)) continue;
            w->installEventFilter(this);
        }
    }


    // 标题栏按钮
    if (ui->min_button) {
        connect(ui->min_button, &QPushButton::clicked, this, &QWidget::showMinimized);
    }
    if (ui->max_button) {
        connect(ui->max_button, &QPushButton::clicked, this, [this]() {
            isMaximized() ? showNormal() : showMaximized();
        });
    }
    if (ui->close_button) {
        connect(ui->close_button, &QPushButton::clicked, this, &QWidget::close);
    }
}

bool LoginWindow::eventFilter(QObject* obj, QEvent* event)
{
    QWidget* w = qobject_cast<QWidget*>(obj);
    const bool inTitleBar = ui->frame_4 && w && (w == ui->frame_4 || ui->frame_4->isAncestorOf(w));
    if (inTitleBar) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto* e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::LeftButton) {
                m_dragging = true;
                m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            if (m_dragging && !isMaximized()) {
                auto* e = static_cast<QMouseEvent*>(event);
                move(e->globalPosition().toPoint() - m_dragOffset);
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            m_dragging = false;
            break;
        }
        case QEvent::MouseButtonDblClick: {
            isMaximized() ? showNormal() : showMaximized();
            return true;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
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

    QString dbErr;
    if (!DbManager::instance().init(&dbErr)) {
        QMessageBox::critical(this, "登录", QString("数据库初始化失败：\n%1\n\n数据库路径：\n%2")
                                          .arg(dbErr, DbManager::instance().databasePath()));
        return;
    }

    if (!DbManager::instance().loginUser(user, pass)) {
        QMessageBox::warning(this, "登录", "用户名或密码错误。");
        return;
    }

    // 登录成功 -> 进入主界面
    if (m_main) {
        m_main->close();
        m_main = nullptr;
    }

    m_main = new MainWindow(user);
    m_main->setAttribute(Qt::WA_DeleteOnClose, true);

    // 退出登录：返回登录页，而不是退出程序
    connect(m_main, &MainWindow::logoutRequested, this, [this]() {
        this->show();
        ui->lineEditPass->clear();
    });
    connect(m_main, &QObject::destroyed, this, [this]() { m_main = nullptr; });

    m_main->show();
    this->hide();
}
