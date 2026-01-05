#include "registerwindow.h"
#include "ui_register_window.h"

#include <QMessageBox>
#include <QPushButton>
#include <QMouseEvent>
#include <QEvent>
#include "dbmanager.h"

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <windowsx.h>
#  pragma comment(lib, "User32.lib")   // ✅ 关键：链接 GetWindowRect 所在库
#endif

RegisterWindow::RegisterWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RegisterWindow)
{
    ui->setupUi(this);

    setupFrameless();
    bindWindowButtons();

    ui->lineEditPass->setEchoMode(QLineEdit::Password);
    ui->lineEditConfirm->setEchoMode(QLineEdit::Password);

    connect(ui->register_pushButton, &QPushButton::clicked, this, &RegisterWindow::handleRegister);
    connect(ui->return_pushButton, &QPushButton::clicked, this, &RegisterWindow::handleReturn);
}

void RegisterWindow::setupFrameless()
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    if (!ui->frame_4) return;
    ui->frame_4->installEventFilter(this);
    const auto children = ui->frame_4->findChildren<QWidget*>();
    for (auto *w : children) {
        if (!w) continue;
        if (w == ui->min_button || w == ui->max_button || w == ui->close_button) continue;
        w->installEventFilter(this);
    }
}

void RegisterWindow::bindWindowButtons()
{
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

bool RegisterWindow::eventFilter(QObject *obj, QEvent *event)
{
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
        isMaximized() ? showNormal() : showMaximized();
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

#ifdef Q_OS_WIN
bool RegisterWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
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
