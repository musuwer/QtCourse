#include "chatclient.h"

#include <QAbstractSocket>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonParseError>

ChatClient::ChatClient(QObject* parent)
    : QObject(parent)
    , m_clientSocket(new QTcpSocket(this))
{
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);
    connect(m_clientSocket, &QTcpSocket::connected, this, &ChatClient::connected);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &ChatClient::disconnected);
    connect(m_clientSocket, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
                emit socketError(m_clientSocket->errorString());
            });
}

bool ChatClient::isConnected() const
{
    return m_clientSocket && m_clientSocket->state() == QAbstractSocket::ConnectedState;
}

void ChatClient::connectToServer(const QHostAddress& address, quint16 port)
{
    if (!m_clientSocket) return;
    if (isConnected()) return;

    m_clientSocket->connectToHost(address, port);
}

void ChatClient::disconnectFromHost()
{
    if (!m_clientSocket) return;
    if (m_clientSocket->state() == QAbstractSocket::UnconnectedState) return;
    m_clientSocket->disconnectFromHost();
}

void ChatClient::sendMessage(const QString& text, const QString& type)
{
    if (!isConnected()) return;

    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return;

    QJsonObject obj;
    obj[QStringLiteral("type")] = type;
    obj[QStringLiteral("text")] = trimmed;

    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    QDataStream stream(m_clientSocket);
    stream.setVersion(QDataStream::Qt_5_12);
    stream << payload;
}

void ChatClient::onReadyRead()
{
    QByteArray jsonData;
    QDataStream socketStream(m_clientSocket);
    socketStream.setVersion(QDataStream::Qt_5_12);

    for (;;) {
        socketStream.startTransaction();
        socketStream >> jsonData;
        if (!socketStream.commitTransaction()) {
            break;
        }

        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) continue;
        if (!doc.isObject()) continue;

        emit jsonReceived(doc.object());
    }
}
