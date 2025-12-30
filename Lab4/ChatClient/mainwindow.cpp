#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QHostAddress>
#include <QJsonValue>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
    m_chatClient = new ChatClient(this);

    connect(m_chatClient,&ChatClient::connected,this,&MainWindow::connectedToServer);
//    connect(m_chatClient,&ChatClient::messageRecived,this,&MainWindow::messageReviced);
    connect(m_chatClient,&ChatClient::jsonReceived,this,&MainWindow::jsonReceived);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_btnLogin_clicked()
{
    m_chatClient->connectToServer(QHostAddress(ui->IPEdit->text()),1967);


}


void MainWindow::on_btnExit_clicked()
{

}


void MainWindow::on_btnSent_clicked()
{
    if(!ui->messageEdit->text().isEmpty()){
        m_chatClient->sendMessage(ui->messageEdit->text());
    }
}


void MainWindow::on_btnLogout_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
    m_chatClient->disconnectFromHost();

    for(auto aItem:ui->UserListWidget->findItems(ui->nameEdit->text(),Qt::MatchExactly)){
        qDebug()<<"auto aItem:ui->U";
        ui->UserListWidget->removeItemWidget(aItem);
        delete aItem;
    }

}

void MainWindow::connectedToServer()
{
    ui->stackedWidget->setCurrentWidget(ui->chatPage);
    m_chatClient->sendMessage(ui->nameEdit->text(),"login");
}

void MainWindow::messageReviced(const QString &sender, const QString &text)
{
    ui->roomTextEdit->append(QString("%1 : %2").arg(sender).arg(text));
}


void MainWindow::jsonReceived(const QJsonObject &docObj)
{
    const QJsonValue typeVal = docObj.value("type");
    if(typeVal.isNull() || !typeVal.isString()){
        return;
    }
    if(typeVal.toString().compare("message",Qt::CaseInsensitive) == 0){
        const QJsonValue textVal = docObj.value("text");
        const QJsonValue senderVal = docObj.value("sender");
        if(textVal.isNull() || !textVal.isString()){
            return;
        }
        if(senderVal.isNull() || !senderVal.isString()){
            return;
        }
         ui->roomTextEdit->append(senderVal.toString()+" : "+textVal.toString());

    }
    if(typeVal.toString().compare("newUser",Qt::CaseInsensitive) == 0){
        qDebug()<<typeVal.toString();
        const QJsonValue nameVal = docObj.value("userName");
        if(nameVal.isNull() || !nameVal.isString()){
            qDebug()<<"nameVal.isNull() || !nameVal.isString()";
            return;
        }
        qDebug()<<"serJoi";
        userJoined(nameVal.toString());
    }
    if(typeVal.toString().compare("userDisconnected",Qt::CaseInsensitive) == 0){
        qDebug()<<typeVal.toString();
        const QJsonValue nameVal = docObj.value("userName");
        if(nameVal.isNull() || !nameVal.isString()){
            qDebug()<<"nameVal.isNull() || !nameVal.isString()";
            return;
        }
        userDelete(nameVal.toString());
    }
    if(typeVal.toString().compare("userList",Qt::CaseInsensitive) == 0){
        qDebug()<<typeVal.toString();
        const QJsonValue userListVal = docObj.value("userList");
        if(userListVal.isNull() || !userListVal.isArray()){
            qDebug()<<"nameVal.isNull() || !nameVal.isString()";
            return;
        }
        qDebug()<<userListVal.toVariant().toStringList();
        userListReceived(userListVal.toVariant().toStringList());
    }
}

void MainWindow::userJoined(const QString &user)
{
    ui->UserListWidget->addItem(user);
}

void MainWindow::userDelete(const QString &user)
{
    for(auto aItem:ui->UserListWidget->findItems(user,Qt::MatchExactly)){
        qDebug()<<"auto aItem:ui->U";
        ui->UserListWidget->removeItemWidget(aItem);
        delete aItem;
    }
}

void MainWindow::userListReceived(const QStringList &list)
{
    ui->UserListWidget->clear();
    ui->UserListWidget->addItems(list);
}

