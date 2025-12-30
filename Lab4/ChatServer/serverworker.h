#ifndef SERVERWORKER_H
#define SERVERWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class ServerWorker : public QObject
{
    Q_OBJECT
public:
    explicit ServerWorker(QObject *parent = nullptr);
    virtual bool setSocketDescriptor(qintptr socketDescriptor); // 修正了拼写建议，但为了兼容你的旧代码，这里保持原样也没事

    const QString &getUserName() const;
    void setUserName(const QString &newUserName);

signals:
    void logMessage(const QString &message);
    void jsonRecived(ServerWorker *sender, const QJsonObject &docObj);
    void disconnectedFromClient();

private:
    QTcpSocket *m_serverSocket;
    QString userName;

public slots:
    void onReadyRead();
    void sendMessage(const QString &text, const QString &type = "message");
    // sendJson 需要被 QMetaObject 调用，所以必须是 slot 或者 Q_INVOKABLE
    void sendJson(const QJsonObject &json);
    void disconnectFromClient(); // 新增：用于彻底断开连接
};

#endif // SERVERWORKER_H
