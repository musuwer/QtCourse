#ifndef SERVERWORKER_H
#define SERVERWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

// 每个客户端连接对应一个 ServerWorker（运行在独立线程中）
class ServerWorker : public QObject
{
    Q_OBJECT
public:
    explicit ServerWorker(QObject* parent = nullptr);

    bool setSocketDescriptor(qintptr socketDescriptor);

    const QString& getUserName() const;
    void setUserName(const QString& newUserName);

signals:
    void logMessage(const QString& message);
    void jsonReceived(ServerWorker* sender, const QJsonObject& docObj);
    void disconnectedFromClient();

public slots:
    void onReadyRead();

    // 发送一条 JSON 消息到当前客户端（跨线程调用时需要是 slot）
    void sendJson(const QJsonObject& json);

    // 让 worker 在自己的线程里断开连接
    void disconnectFromClient();

private:
    QTcpSocket* m_serverSocket = nullptr;
    QString m_userName;
};

#endif // SERVERWORKER_H
