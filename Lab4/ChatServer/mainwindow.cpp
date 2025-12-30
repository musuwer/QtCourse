#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QHostAddress>
#include <QNetworkInterface> // 新增：用于获取本机IP地址

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 创建服务器对象
    m_chatServer = new chatServer(this);

    // 连接日志信号
    connect(m_chatServer, &chatServer::logMessage, this, &MainWindow::logMessage);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startStopButton_clicked()
{
    // 判断当前服务器状态
    if(m_chatServer->isListening()){
        // --- 当前正在监听，点击执行停止逻辑 ---

        m_chatServer->stopServer();

        // 更新界面
        ui->startStopButton->setText("启动服务器");
        logMessage("========== 服务器已停止 ==============");
    }
    else{
        // --- 当前未监听，点击执行启动逻辑 ---

        // 尝试监听任意地址的 1967 端口
        if(!m_chatServer->listen(QHostAddress::Any, 1967)){
            QMessageBox::critical(this, "启动失败", "无法启动服务器！\n请检查端口 1967 是否被占用。");
            return;
        }

        // 更新界面
        ui->startStopButton->setText("停止服务器");
        logMessage("========== 服务器已启动 ==============");
        logMessage("监听端口: 1967");

        // 【优化功能】自动打印本机IP地址，方便在客户端输入
        logMessage("可用IP地址列表:");
        const QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
        for (const QHostAddress &entry : ipAddressesList) {
            // 只显示 IPv4 且不是 127.0.0.1 的地址
            if (entry != QHostAddress::LocalHost && entry.toIPv4Address()) {
                logMessage(" -> " + entry.toString());
            }
        }
    }
}

void MainWindow::logMessage(const QString &message)
{
    ui->logEdit->appendPlainText(message);
}
