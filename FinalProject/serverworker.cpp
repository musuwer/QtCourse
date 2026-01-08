#include "serverworker.h"


#include <QAbstractSocket>
#include <QByteArray>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonParseError>

ServerWorker::ServerWorker(QObject* parent)
    : QObject(parent)
    , m_serverSocket(new QTcpSocket(this))
{
    connect(m_serverSocket, &QTcpSocket::readyRead, this, &ServerWorker::onReadyRead);
    connect(m_serverSocket, &QTcpSocket::disconnected, this, &ServerWorker::disconnectFromClient);
}

bool ServerWorker::setSocketDescriptor(qintptr socketDescriptor)
{
    return m_serverSocket->setSocketDescriptor(socketDescriptor);
}

const QString& ServerWorker::getUserName() const
{
    return m_userName;
}

void ServerWorker::setUserName(const QString& newUserName)
{
    m_userName = newUserName;
}

void ServerWorker::disconnectFromClient()
{
    if (m_serverSocket->state() != QAbstractSocket::UnconnectedState) {
        m_serverSocket->disconnectFromHost();
    }
    emit disconnectedFromClient();
}

void ServerWorker::onReadyRead()
{
    QByteArray jsonData;
    QDataStream socketStream(m_serverSocket);
    socketStream.setVersion(QDataStream::Qt_5_12);

    for (;;) {
        socketStream.startTransaction();
        socketStream >> jsonData;
        if (!socketStream.commitTransaction()) {
            break;
        }

        QJsonParseError parseError{};
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            continue;
        }
        if (!jsonDoc.isObject()) {
            continue;
        }

        emit jsonReceived(this, jsonDoc.object());
    }
}

void ServerWorker::sendJson(const QJsonObject& json)
{
    if (m_serverSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    const QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QDataStream socketStream(m_serverSocket);
    socketStream.setVersion(QDataStream::Qt_5_12);
    socketStream << jsonData;
}
