#include "mainwindow.h"
#include "ui_main_window.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QEvent>

#include "dbmanager.h"
#include "homewindow.h"
#include "achievementwindow.h"
#include "logrecordwindow.h"
#include "moodcalendarwindow.h"
#include "messageuserwindow.h"
#include "messageadminwindow.h"
#include "aboutwindow.h"
#include "loginwindow.h"

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <windowsx.h>
#  pragma comment(lib, "User32.lib")   // ✅ 关键：链接 GetWindowRect 所在库
#endif

MainWindow::MainWindow(const QString& username, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_username(username)
{
    ui->setupUi(this);

    setupFrameless();
    bindWindowButtons();

    m_userId = DbManager::instance().getUserId(m_username);
    m_role = DbManager::instance().getUserRole(m_username);

    initUi();
    initPages();
    connectSignals();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initUi()
{
    // 系统命名（基础版）
    setWindowTitle("CampusMate · 学途管家");

    ui->current_username_label->setText(m_username);
    ui->current_role_label->setText(m_role.isEmpty() ? "user" : m_role);
}

void MainWindow::initPages()
{
    // 清空 ui 中默认 page/page_2（如果存在）
    while (ui->stackedWidget->count() > 0) {
        QWidget* w = ui->stackedWidget->widget(0);
        ui->stackedWidget->removeWidget(w);
        w->deleteLater();
    }

    m_home = new HomeWindow(m_userId, m_username, m_role, this);
    m_achievement = new AchievementWindow(m_userId, m_username, m_role, this);
    m_log = new LogRecordWindow(m_userId, m_username, m_role, this);
    m_mood = new MoodCalendarWindow(m_userId, m_username, m_role, this);

    if (m_role == "admin") {
        m_messagePage = new MessageAdminWindow(m_username, this);
    } else {
        m_messagePage = new MessageUserWindow(m_username, this);
    }

    m_about = new AboutWindow(this);

    // 顺序必须与 listWidget 项一致
    ui->stackedWidget->addWidget(m_home);        // 0 主页
    ui->stackedWidget->addWidget(m_achievement); // 1 成就管理
    ui->stackedWidget->addWidget(m_log);         // 2 事件记录
    ui->stackedWidget->addWidget(m_mood);        // 3 心情日历
    ui->stackedWidget->addWidget(m_messagePage); // 4 反馈交流
    ui->stackedWidget->addWidget(m_about);       // 5 关于

    ui->listWidget->setCurrentRow(0);
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::connectSignals()
{
    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &MainWindow::onNavChanged);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
}

void MainWindow::setupFrameless()
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

void MainWindow::bindWindowButtons()
{
    if (ui->min_button) {
        disconnect(ui->min_button, nullptr, this, nullptr);
        connect(ui->min_button, &QPushButton::clicked, this, &MainWindow::onMinClicked);
    }
    if (ui->max_button) {
        disconnect(ui->max_button, nullptr, this, nullptr);
        connect(ui->max_button, &QPushButton::clicked, this, &MainWindow::onMaxClicked);
    }
    if (ui->close_button) {
        disconnect(ui->close_button, nullptr, this, nullptr);
        connect(ui->close_button, &QPushButton::clicked, this, &MainWindow::onCloseClicked);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
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

    return QMainWindow::eventFilter(obj, event);
}

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    if (isMaximized()) {
        return QMainWindow::nativeEvent(eventType, message, result);
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

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::onNavChanged(int row)
{
    if (row < 0 || row >= ui->stackedWidget->count()) return;
    ui->stackedWidget->setCurrentIndex(row);
}

void MainWindow::onLogoutClicked()
{
    const auto ret = QMessageBox::question(this, "退出登录", "确定要退出当前账号吗？");
    if (ret != QMessageBox::Yes) return;

    // 切换用户：回到登录页面（不退出程序）
    auto* lw = new LoginWindow();
    lw->setAttribute(Qt::WA_DeleteOnClose);
    lw->show();
    close();
}

void MainWindow::onMinClicked()
{
    showMinimized();
}

void MainWindow::onMaxClicked()
{
    isMaximized() ? showNormal() : showMaximized();
}

void MainWindow::onCloseClicked()
{
    close();
}
