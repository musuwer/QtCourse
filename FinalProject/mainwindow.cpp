#include "mainwindow.h"
#include "ui_main_window.h"

#include <QMessageBox>

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
    m_achievement = new AchievementWindow(m_userId, m_username, this);
    m_log = new LogRecordWindow(m_userId, m_username, this);
    m_mood = new MoodCalendarWindow(m_userId, m_username, this);

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
