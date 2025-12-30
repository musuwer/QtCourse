#include "serverworker.h"
#include <QByteArray>
#include <QDataStream>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

ServerWorker::ServerWorker(QObject *parent) : QObject(parent)
{
    m_serverSocket = new QTcpSocket(this);
    connect(m_serverSocket, &QTcpSocket::readyRead, this, &ServerWorker::onReadyRead);
    // 这里连接 disconnected 信号到槽函数
    connect(m_serverSocket, &QTcpSocket::disconnected, this, &ServerWorker::disconnectFromClient);
}

bool ServerWorker::setSocketDescriptor(qintptr socketDescriptor)
{
    return m_serverSocket->setSocketDescriptor(socketDescriptor);
}

const QString &ServerWorker::getUserName() const
{
    return userName;
}

void ServerWorker::setUserName(const QString &newUserName)
{
    userName = newUserName;
}

void ServerWorker::disconnectFromClient()
{
    m_serverSocket->disconnectFromHost();
    // 发送信号通知 Thread 退出
    emit disconnectedFromClient();
}

void ServerWorker::onReadyRead()
{
    QByteArray jsonData;
    QDataStream socketStream(m_serverSocket);
    socketStream.setVersion(QDataStream::Qt_5_12);

    for(;;){
        socketStream.startTransaction();
        socketStream >> jsonData;
        if(socketStream.commitTransaction()){
            // 调试用：打印收到的原始数据
            // emit logMessage(QString::fromUtf8(jsonData));

            QJsonParseError parseError;
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
            if(parseError.error == QJsonParseError::NoError){
                if(jsonDoc.isObject()){
                    // 将收到的 JSON 对象传给 chatServer 处理
                    emit jsonRecived(this, jsonDoc.object());
                }
            }
        }
        else{
            break;
        }
    }
}

void ServerWorker::sendMessage(const QString &text, const QString &type)
{
    if(m_serverSocket->state() != QAbstractSocket::ConnectedState){
        return;
    }
    if(!text.isEmpty()){
        QDataStream serverStream(m_serverSocket);
        serverStream.setVersion(QDataStream::Qt_5_12);

        QJsonObject message;
        message["type"] = type;
        message["text"] = text;
        serverStream << QJsonDocument(message).toJson();
    }
}

void ServerWorker::sendJson(const QJsonObject &json)
{
    const QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);

    // 这里的 emit logMessage 可能会比较频繁，可以根据需要注释掉
    emit logMessage(QLatin1String("Sending to ") + getUserName() + QLatin1String(" - ") + QString::fromUtf8(jsonData));

    QDataStream socketStream(m_serverSocket);
    socketStream.setVersion(QDataStream::Qt_5_12);
    socketStream << jsonData;
}
