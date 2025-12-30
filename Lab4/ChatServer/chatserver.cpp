#include "chatserver.h"
#include "serverworker.h"
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>      // 必须包含
#include <QMetaObject>  // 必须包含

chatServer::chatServer(QObject *parent):
    QTcpServer(parent)
{
}

void chatServer::incomingConnection(qintptr socketDescriptor)
{
    // 1. 创建 Worker (注意：不能有父对象，否则无法移动线程)
    ServerWorker *worker = new ServerWorker;

    // 2. 设置 Socket 描述符
    if (!worker->setSocketDescriptor(socketDescriptor)) {
        worker->deleteLater();
        return;
    }

    // 3. 创建新线程
    QThread *thread = new QThread;

    // 4. 将 Worker 移动到新线程
    worker->moveToThread(thread);

    // 5. 线程生命周期管理
    // 当线程结束时，销毁 worker
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    // 当 worker 断开连接时，退出线程
    connect(worker, &ServerWorker::disconnectedFromClient, thread, &QThread::quit);
    // 线程结束后，销毁线程对象自身
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    // 6. 业务逻辑连接 (Qt::QueuedConnection 会自动处理跨线程信号)
    connect(worker, &ServerWorker::logMessage, this, &chatServer::logMessage);
    connect(worker, &ServerWorker::jsonRecived, this, &chatServer::jsonRecived);
    connect(worker, &ServerWorker::disconnectedFromClient, this, std::bind(&chatServer::userDisconnected, this, worker));

    // 7. 启动线程
    thread->start();

    // 添加到列表
    m_clients.append(worker);
    emit logMessage("新用户已连接");
}

void chatServer::broadcast(const QJsonObject &message, ServerWorker *exclude)
{
    // 遍历所有客户端
    for(ServerWorker * worker : m_clients){
        // 如果 worker 不是被排除的对象 (exclude)
        if(worker != exclude){
            // 【重要】跨线程调用 sendJson
            // 直接调用 worker->sendJson() 会导致在主线程操作子线程的 Socket，引发错误
            QMetaObject::invokeMethod(worker, "sendJson",
                                      Qt::QueuedConnection,
                                      Q_ARG(QJsonObject, message));
        }
    }
}

void chatServer::stopServer()
{
    // 停止服务时断开所有连接
    for(auto worker : m_clients) {
        // 让 worker 在自己的线程中断开
        QMetaObject::invokeMethod(worker, "disconnectFromClient", Qt::QueuedConnection);
    }
    close();
}

void chatServer::jsonRecived(ServerWorker *sender, const QJsonObject &docObj)
{
    const QJsonValue typeVal = docObj.value("type");
    if(typeVal.isNull() || !typeVal.isString()){
        return;
    }

    // --- 1. 处理普通消息 ---
    if(typeVal.toString().compare("message", Qt::CaseInsensitive) == 0){
        const QJsonValue textVal = docObj.value("text");
        if(textVal.isNull() || !textVal.isString()){
            return;
        }
        const QString text = textVal.toString().trimmed();
        if(text.isEmpty()){
            return;
        }

        QJsonObject message;
        message["type"] = "message";
        message["text"] = text;
        message["sender"] = sender->getUserName();

        // 【核心修复】传入 nullptr，表示不排除任何人
        // 这样 A 发消息，服务器会转发给 A 和 B，实现了两边同步显示
        broadcast(message, nullptr);
    }
    // --- 2. 处理登录消息 ---
    else if(typeVal.toString().compare("login", Qt::CaseInsensitive) == 0){
        const QJsonValue nameVal = docObj.value("text");
        if(nameVal.isNull() || !nameVal.isString()){
            return;
        }

        // 设置当前 Worker 的用户名
        sender->setUserName(nameVal.toString());

        // 广播新用户加入 (也传 nullptr，这样自己也能看到“我加入了聊天室”的提示)
        QJsonObject connectedMessage;
        connectedMessage["type"] = "newUser";
        connectedMessage["userName"] = nameVal.toString();
        broadcast(connectedMessage, nullptr);

        // 构建用户列表
        QJsonObject userListMessage;
        userListMessage["type"] = "userList";
        QJsonArray userList;
        for(ServerWorker *worker : m_clients){
            if(worker == sender){
                userList.append(worker->getUserName() + " (Me)");
            }
            else{
                userList.append(worker->getUserName());
            }
        }
        userListMessage["userList"] = userList;

        // 单独把用户列表发回给登录者 (使用 invokeMethod 确保线程安全)
        QMetaObject::invokeMethod(sender, "sendJson",
                                  Qt::QueuedConnection,
                                  Q_ARG(QJsonObject, userListMessage));
    }
}
void chatServer::userDisconnected(ServerWorker *sender)
{
    m_clients.removeAll(sender);
    const QString userName = sender->getUserName();

    if(!userName.isEmpty()){
        QJsonObject disconnectedMessage;
        disconnectedMessage["type"] = "userDisconnected";
        disconnectedMessage["userName"] = userName;
        // 通知剩余的所有人
        broadcast(disconnectedMessage, nullptr);
        emit logMessage(userName + " disconnected");
    }

    // worker 会通过之前绑定的 deleteLater 自动析构，这里不需要手动 delete
}
