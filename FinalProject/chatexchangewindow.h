#ifndef CHATEXCHANGEWINDOW_H
#define CHATEXCHANGEWINDOW_H

#include <QWidget>
#include <QString>
#include <QJsonObject>

class ChatServer;
class ChatClient;

namespace Ui {
class ChatExchangeWindow;
}

class ChatExchangeWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ChatExchangeWindow(const QString& username, QWidget* parent = nullptr);
    ~ChatExchangeWindow();

private slots:
    void onStartServerClicked();
    void onStopServerClicked();

    void onConnectClicked();
    void onDisconnectClicked();

    void onSendClicked();

    void onClientConnected();
    void onClientDisconnected();
    void onClientError(const QString& err);
    void onJsonReceived(const QJsonObject& obj);

    void onUserSelectionChanged();

    void onServerLog(const QString& msg);

private:
    void setClientUiConnected(bool connected);
    void appendSystemMessage(const QString& text);
    void appendChatMessage(const QString& sender, const QString& text, bool isSelf);
    void refreshOnlineCount();
    void addUserToList(const QString& userName);
    void removeUserFromList(const QString& userName);
    void setCurrentPeer(const QString& userName);
    void appendServerLogLine(const QString& line);

private:
    Ui::ChatExchangeWindow* ui;
    QString m_username;

    ChatServer* m_server = nullptr;
    ChatClient* m_client = nullptr;

    QString m_currentPeer; // UI 展示用（群聊 / 选中用户）
};

#endif // CHATEXCHANGEWINDOW_H
