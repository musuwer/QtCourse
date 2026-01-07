#include "mainwindow.h"
#include "ui_main_window.h"

#include <QMessageBox>
#include <QEvent>
#include <QMouseEvent>
#include <QAbstractButton>

#include "dbmanager.h"
#include "homewindow.h"
#include "achievementwindow.h"
#include "logrecordwindow.h"
#include "moodcalendarwindow.h"
#include "messageuserwindow.h"
#include "messageadminwindow.h"
#include "aboutwindow.h"

MainWindow::MainWindow(const QString& username, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_username(username)
{
    ui->setupUi(this);

    initFrameless();

    m_userId = DbManager::instance().getUserId(m_username);
    m_role = DbManager::instance().getUserRole(m_username);

    initUi();
    initPages();
    connectSignals();
}

void MainWindow::initFrameless()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    if (ui->frame_4) {
        ui->frame_4->installEventFilter(this);

        // ✅ 关键：标题栏的非按钮子控件也装 eventFilter，让“点到文字/空白”也能拖
        const auto widgets = ui->frame_4->findChildren<QWidget*>();
        for (QWidget* w : widgets) {
            if (!w) continue;
            if (qobject_cast<QAbstractButton*>(w)) continue; // 不拦截按钮点击
            w->installEventFilter(this);
        }
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
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
    return QMainWindow::eventFilter(obj, event);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initUi()
{
    setWindowTitle("Trip Memory · 旅忆");

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

    connect(ui->min_button, &QPushButton::clicked, this, &MainWindow::onMinClicked);
    connect(ui->max_button, &QPushButton::clicked, this, &MainWindow::onMaxClicked);
    connect(ui->close_button, &QPushButton::clicked, this, &MainWindow::onCloseClicked);
}

void MainWindow::onNavChanged(int row)
{
    if (row < 0 || row >= ui->stackedWidget->count()) return;
    ui->stackedWidget->setCurrentIndex(row);
}

void MainWindow::onLogoutClicked()
{
    const auto ret = QMessageBox::question(this, "退出登录", "确定要退出当前账号吗？");
    if (ret != QMessageBox::Yes) return;

    emit logoutRequested();
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
