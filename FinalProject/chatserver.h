#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QTcpServer>
#include <QJsonObject>
#include <QVector>

#include "serverworker.h"

// 聊天服务器：维护在线客户端列表，并广播消息
class ChatServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ChatServer(QObject* parent = nullptr);

    bool startListen(quint16 port, QString* err = nullptr);

public slots:
    void stopServer();

private slots:
    void jsonReceived(ServerWorker* sender, const QJsonObject& docObj);
    void userDisconnected(ServerWorker* sender);

signals:
    void logMessage(const QString& message);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    void broadcast(const QJsonObject& message, ServerWorker* exclude = nullptr);

private:
    QVector<ServerWorker*> m_clients;
};

#endif // CHATSERVER_H
