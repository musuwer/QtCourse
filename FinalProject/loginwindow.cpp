#include "loginwindow.h"
#include "ui_login_window.h"

#include <QMessageBox>
#include <QPushButton>
#include <QMouseEvent>
#include <QEvent>

#include "registerwindow.h"
#include "mainwindow.h"
#include "dbmanager.h"

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <windowsx.h>
#  pragma comment(lib, "User32.lib")   // ✅ 关键：链接 GetWindowRect 所在库
#endif


LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);

    setupFrameless();
    bindWindowButtons();

    ui->lineEditPass->setEchoMode(QLineEdit::Password);

    connect(ui->login_pushButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(ui->btnRegister, &QPushButton::clicked, this, &LoginWindow::openRegister);

    // 第三方登录（基础版仅弹提示）
    if (ui->qq_login) connect(ui->qq_login, &QPushButton::clicked, this, &LoginWindow::otherLoginNotReady);
    if (ui->wechat_login) connect(ui->wechat_login, &QPushButton::clicked, this, &LoginWindow::otherLoginNotReady);
    if (ui->weibo_login) connect(ui->weibo_login, &QPushButton::clicked, this, &LoginWindow::otherLoginNotReady);

    // 兼容：如果将来 UI 中新增了“其他登录方式”按钮，也能自动绑定
    bindOtherLoginButtonsByText();
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::setupFrameless()
{
    // 去掉系统标题栏/边框（你界面自己画了最小化/最大化/关闭）
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    // 让顶部栏能拖动：对 frame_4 及其子控件（除按钮外）安装 eventFilter
    if (!ui->frame_4) return;
    ui->frame_4->installEventFilter(this);
    const auto children = ui->frame_4->findChildren<QWidget*>();
    for (auto *w : children) {
        if (!w) continue;
        if (w == ui->min_button || w == ui->max_button || w == ui->close_button) continue;
        w->installEventFilter(this);
    }
}

void LoginWindow::bindWindowButtons()
{
    // Designer 里原来连到了 showNormal()/showFullScreen()，会导致行为不对，这里统一断开再绑定
    if (ui->min_button) {
        disconnect(ui->min_button, nullptr, this, nullptr);
        connect(ui->min_button, &QPushButton::clicked, this, &QWidget::showMinimized);
    }
    if (ui->max_button) {
        disconnect(ui->max_button, nullptr, this, nullptr);
        connect(ui->max_button, &QPushButton::clicked, this, [this] {
            isMaximized() ? showNormal() : showMaximized();
        });
    }
    if (ui->close_button) {
        disconnect(ui->close_button, nullptr, this, nullptr);
        connect(ui->close_button, &QPushButton::clicked, this, &QWidget::close);
    }
}

bool LoginWindow::eventFilter(QObject *obj, QEvent *event)
{
    // 仅处理拖动顶部栏
    if (event->type() == QEvent::MouseButtonPress) {
        auto *e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            return true;
        }
    } else if (event->type() == QEvent::MouseMove) {
        if (m_dragging && !isMaximized()) {
            auto *e = static_cast<QMouseEvent*>(event);
            move(e->globalPosition().toPoint() - m_dragPos);
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_dragging = false;
        return true;
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        // 双击顶部栏切换最大化
        isMaximized() ? showNormal() : showMaximized();
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

#ifdef Q_OS_WIN
bool LoginWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    // 允许无边框窗口从边缘缩放
    if (isMaximized()) {
        return QWidget::nativeEvent(eventType, message, result);
    }

    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG *msg = static_cast<MSG*>(message);
        if (msg->message == WM_NCHITTEST) {
            const int border = 8;
            RECT winrect;
            GetWindowRect(reinterpret_cast<HWND>(winId()), &winrect);

            const long x = GET_X_LPARAM(msg->lParam);
            const long y = GET_Y_LPARAM(msg->lParam);

            const bool left   = x >= winrect.left && x < winrect.left + border;
            const bool right  = x <= winrect.right && x > winrect.right - border;
            const bool top    = y >= winrect.top && y < winrect.top + border;
            const bool bottom = y <= winrect.bottom && y > winrect.bottom - border;

            if (top && left)    { *result = HTTOPLEFT; return true; }
            if (top && right)   { *result = HTTOPRIGHT; return true; }
            if (bottom && left) { *result = HTBOTTOMLEFT; return true; }
            if (bottom && right){ *result = HTBOTTOMRIGHT; return true; }
            if (left)           { *result = HTLEFT; return true; }
            if (right)          { *result = HTRIGHT; return true; }
            if (top)            { *result = HTTOP; return true; }
            if (bottom)         { *result = HTBOTTOM; return true; }
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}
#endif

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
