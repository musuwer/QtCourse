#include "chatserver.h"


#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMetaObject>
#include <QThread>

ChatServer::ChatServer(QObject* parent)
    : QTcpServer(parent)
{
}

bool ChatServer::startListen(quint16 port, QString* err)
{
    if (isListening()) {
        if (err) *err = QStringLiteral("服务器已在运行");
        return true;
    }

    if (!listen(QHostAddress::Any, port)) {
        if (err) *err = errorString();
        return false;
    }

    emit logMessage(QStringLiteral("服务器已启动，端口：%1").arg(serverPort()));
    return true;
}

void ChatServer::incomingConnection(qintptr socketDescriptor)
{
    // 1) Worker 不能有父对象，否则 moveToThread 会失败
    auto* worker = new ServerWorker;

    if (!worker->setSocketDescriptor(socketDescriptor)) {
        worker->deleteLater();
        return;
    }

    // 2) 每个连接一个线程，避免主线程阻塞
    auto* thread = new QThread;
    worker->moveToThread(thread);

    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &ServerWorker::disconnectedFromClient, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    // 3) 业务信号（跨线程自动排队）
    connect(worker, &ServerWorker::logMessage, this, &ChatServer::logMessage);
    connect(worker, &ServerWorker::jsonReceived, this, &ChatServer::jsonReceived);
    connect(worker, &ServerWorker::disconnectedFromClient, this, [this, worker] {
        userDisconnected(worker);
    });

    thread->start();

    m_clients.append(worker);
    emit logMessage(QStringLiteral("新用户已连接（当前在线：%1）").arg(m_clients.size()));
}

void ChatServer::broadcast(const QJsonObject& message, ServerWorker* exclude)
{
    for (ServerWorker* worker : m_clients) {
        if (!worker) continue;
        if (exclude && worker == exclude) continue;

        // 重要：跨线程调用 sendJson
        QMetaObject::invokeMethod(worker, "sendJson", Qt::QueuedConnection, Q_ARG(QJsonObject, message));
    }
}

void ChatServer::stopServer()
{
    // 停止服务时断开所有连接
    for (ServerWorker* worker : m_clients) {
        if (!worker) continue;
        QMetaObject::invokeMethod(worker, "disconnectFromClient", Qt::QueuedConnection);
    }

    m_clients.clear();

    if (isListening()) {
        close();
    }
    emit logMessage(QStringLiteral("服务器已停止"));
}

void ChatServer::jsonReceived(ServerWorker* sender, const QJsonObject& docObj)
{
    const QJsonValue typeVal = docObj.value(QStringLiteral("type"));
    if (!typeVal.isString()) return;

    const QString type = typeVal.toString();

    // 1) 普通消息
    if (type.compare(QStringLiteral("message"), Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = docObj.value(QStringLiteral("text"));
        if (!textVal.isString()) return;

        const QString text = textVal.toString().trimmed();
        if (text.isEmpty()) return;

        QJsonObject message;
        message[QStringLiteral("type")] = QStringLiteral("message");
        message[QStringLiteral("text")] = text;
        message[QStringLiteral("sender")] = sender ? sender->getUserName() : QStringLiteral("unknown");

        // 广播给所有人（包括自己），保证两边都同步显示
        broadcast(message, nullptr);
        return;
    }

    // 2) 登录
    if (type.compare(QStringLiteral("login"), Qt::CaseInsensitive) == 0) {
        const QJsonValue nameVal = docObj.value(QStringLiteral("text"));
        if (!nameVal.isString()) return;

        const QString userName = nameVal.toString().trimmed();
        if (userName.isEmpty()) return;

        if (sender) sender->setUserName(userName);

        // 广播新用户加入（自己也能看到）
        QJsonObject connectedMessage;
        connectedMessage[QStringLiteral("type")] = QStringLiteral("newUser");
        connectedMessage[QStringLiteral("userName")] = userName;
        broadcast(connectedMessage, nullptr);

        // 构建用户列表（发给所有在线用户）
        QJsonObject userListMessage;
        userListMessage[QStringLiteral("type")] = QStringLiteral("userList");

        QJsonArray userList;
        for (ServerWorker* worker : m_clients) {
            if (!worker) continue;
            const QString n = worker->getUserName().trimmed();
            if (n.isEmpty()) continue;
            userList.append(n);
        }
        userListMessage[QStringLiteral("userList")] = userList;
        broadcast(userListMessage, nullptr);
        return;
    }
}

void ChatServer::userDisconnected(ServerWorker* sender)
{
    if (!sender) return;

    m_clients.removeAll(sender);

    const QString userName = sender->getUserName();
    if (!userName.isEmpty()) {
        QJsonObject disconnectedMessage;
        disconnectedMessage[QStringLiteral("type")] = QStringLiteral("userDisconnected");
        disconnectedMessage[QStringLiteral("userName")] = userName;
        broadcast(disconnectedMessage, nullptr);
    }

    // 同步更新用户列表给所有在线用户
    {
        QJsonObject userListMessage;
        userListMessage[QStringLiteral("type")] = QStringLiteral("userList");
        QJsonArray userList;
        for (ServerWorker* worker : m_clients) {
            if (!worker) continue;
            const QString n = worker->getUserName().trimmed();
            if (n.isEmpty()) continue;
            userList.append(n);
        }
        userListMessage[QStringLiteral("userList")] = userList;
        broadcast(userListMessage, nullptr);
    }

    emit logMessage(QStringLiteral("%1 disconnected（当前在线：%2）").arg(userName).arg(m_clients.size()));
}
