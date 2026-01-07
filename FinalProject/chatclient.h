#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonObject>

// TCP(JSON) 聊天客户端：
// - 使用 QDataStream 发送/接收 QByteArray(JSON)
// - 与 Lab4 的 ChatServer/ServerWorker 协议保持一致
class ChatClient : public QObject
{
    Q_OBJECT
public:
    explicit ChatClient(QObject* parent = nullptr);

    bool isConnected() const;

public slots:
    void connectToServer(const QHostAddress& address, quint16 port);
    void disconnectFromHost();
    void sendMessage(const QString& text, const QString& type = QStringLiteral("message"));

private slots:
    void onReadyRead();

signals:
    void connected();
    void disconnected();
    void socketError(const QString& errorString);
    void jsonReceived(const QJsonObject& docObj);

private:
    QTcpSocket* m_clientSocket = nullptr;
};

#endif // CHATCLIENT_H
