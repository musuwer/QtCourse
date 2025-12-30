#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QTcpServer>
#include <QObject>
#include <QVector>
#include "serverworker.h"

class chatServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit chatServer(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

    // 存储所有客户端 Worker 的列表
    QVector<ServerWorker *> m_clients;

    // 广播函数
    void broadcast(const QJsonObject &message, ServerWorker *exclude);

public slots:
    void stopServer();
    // 注意：这里保留了你原来的拼写 jsonRecived
    void jsonRecived(ServerWorker *sender, const QJsonObject &docObj);
    void userDisconnected(ServerWorker *sender);

signals:
    void logMessage(const QString &message);
};

#endif // CHATSERVER_H
