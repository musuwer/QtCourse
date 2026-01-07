#include "chatexchangewindow.h"
#include "ui_chat_exchange_window.h"

#include "chatclient.h"
#include "chatserver.h"

#include <QDateTime>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonValue>
#include <QMessageBox>
#include <QFont>
#include <QScrollBar>

#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <QIcon>
#include <QSize>
#include <QVector>


namespace {
static QString avatarInitial(const QString& name)
{
    QString n = name.trimmed();
    if (n.isEmpty()) return QStringLiteral("?");
    const QString ch = n.left(1);
    return ch.toUpper();
}

static QColor avatarColorFor(const QString& name)
{
    static const QVector<QColor> palette = {
        QColor("#FF8A80"), // 珊瑚红
        QColor("#FFB74D"), // 橙
        QColor("#FFD54F"), // 黄
        QColor("#81C784"), // 绿
        QColor("#4DB6AC"), // 青
        QColor("#64B5F6"), // 蓝
        QColor("#9575CD"), // 紫
        QColor("#F06292")  // 粉
    };
    const uint h = qHash(name);
    return palette[static_cast<int>(h % static_cast<uint>(palette.size()))];
}

static QPixmap avatarPixmap(const QString& name, int size)
{
    const int s = qMax(18, size);
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor bg = avatarColorFor(name);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawEllipse(QRectF(0, 0, s, s));

    QFont f = p.font();
    f.setBold(true);
    f.setPointSizeF(s * 0.45);
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(pm.rect(), Qt::AlignCenter, avatarInitial(name));

    return pm;
}

static QIcon avatarIcon(const QString& name, int size)
{
    return QIcon(avatarPixmap(name, size));
}

static QString avatarHtml(const QString& name, int size)
{
    const int s = qMax(18, size);
    const QString color = avatarColorFor(name).name();
    const QString init = avatarInitial(name).toHtmlEscaped();

    return QString(
        "<span style='display:inline-block; width:%1px; height:%1px; "
        "border-radius:%2px; background:%3; color:#ffffff; font-weight:800; "
        "font-size:%4px; line-height:%1px; text-align:center;'>%5</span>")
        .arg(s)
        .arg(s / 2)
        .arg(color)
        .arg(qMax(10, static_cast<int>(s * 0.45)))
        .arg(init);
}
} // namespace

ChatExchangeWindow::ChatExchangeWindow(const QString& username, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ChatExchangeWindow)
    , m_username(username)
    , m_client(new ChatClient(this))
{
    ui->setupUi(this);

    ui->nicknameLabel->setText(m_username);
    // 头像（圆形 / 首字母）
    ui->userListWidget->setIconSize(QSize(28, 28));
    if (ui->nicknameAvatarLabel) {
        ui->nicknameAvatarLabel->setPixmap(avatarPixmap(m_username, 28));
    }

    // 让聊天区域更像聊天窗口（支持富文本气泡）
    ui->chatTextEdit->setReadOnly(true);
    ui->chatTextEdit->setUndoRedoEnabled(false);
    ui->chatTextEdit->setAcceptRichText(true);
    ui->chatTextEdit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    ui->messageLineEdit->setPlaceholderText(QStringLiteral("输入消息...（Enter 发送）"));
    ui->serverIpLineEdit->setPlaceholderText(QStringLiteral("例如：127.0.0.1 或局域网IP"));

    setCurrentPeer(QStringLiteral("群聊"));
    refreshOnlineCount();

    // 客户端信号
    connect(m_client, &ChatClient::connected, this, &ChatExchangeWindow::onClientConnected);
    connect(m_client, &ChatClient::disconnected, this, &ChatExchangeWindow::onClientDisconnected);
    connect(m_client, &ChatClient::socketError, this, &ChatExchangeWindow::onClientError);
    connect(m_client, &ChatClient::jsonReceived, this, &ChatExchangeWindow::onJsonReceived);

    // 按钮
    connect(ui->startServerButton, &QPushButton::clicked, this, &ChatExchangeWindow::onStartServerClicked);
    connect(ui->stopServerButton, &QPushButton::clicked, this, &ChatExchangeWindow::onStopServerClicked);

    connect(ui->connectButton, &QPushButton::clicked, this, &ChatExchangeWindow::onConnectClicked);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &ChatExchangeWindow::onDisconnectClicked);

    connect(ui->sendButton, &QPushButton::clicked, this, &ChatExchangeWindow::onSendClicked);
    connect(ui->messageLineEdit, &QLineEdit::returnPressed, this, &ChatExchangeWindow::onSendClicked);

    connect(ui->userListWidget, &QListWidget::itemSelectionChanged, this, &ChatExchangeWindow::onUserSelectionChanged);

    setClientUiConnected(false);
}

ChatExchangeWindow::~ChatExchangeWindow()
{
    if (m_client) {
        m_client->disconnectFromHost();
    }
    if (m_server) {
        m_server->stopServer();
    }
    delete ui;
}

void ChatExchangeWindow::onStartServerClicked()
{
    if (m_server) {
        QMessageBox::information(this, "提示", "服务器已在运行。");
        return;
    }

    const quint16 port = static_cast<quint16>(ui->portSpinBox->value());

    m_server = new ChatServer(this);
    connect(m_server, &ChatServer::logMessage, this, &ChatExchangeWindow::onServerLog);

    QString err;
    if (!m_server->startListen(port, &err)) {
        QMessageBox::warning(this, "启动失败", err);
        m_server->deleteLater();
        m_server = nullptr;
        return;
    }

    ui->serverStatusLabel->setText(QString("状态：运行中（端口 %1）").arg(port));
    ui->startServerButton->setEnabled(false);
    ui->stopServerButton->setEnabled(true);

    appendServerLogLine(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), QString("服务器启动成功，端口=%1").arg(port)));
}

void ChatExchangeWindow::onStopServerClicked()
{
    if (!m_server) return;

    m_server->stopServer();
    m_server->deleteLater();
    m_server = nullptr;

    ui->serverStatusLabel->setText("状态：未启动");
    ui->startServerButton->setEnabled(true);
    ui->stopServerButton->setEnabled(false);

    appendServerLogLine(QString("[%1] 服务器已停止").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

void ChatExchangeWindow::onConnectClicked()
{
    if (!m_client) return;
    if (m_client->isConnected()) {
        QMessageBox::information(this, "提示", "已连接，无需重复连接。");
        return;
    }

    const QString ip = ui->serverIpLineEdit->text().trimmed();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入服务器IP。");
        return;
    }

    QHostAddress addr;
    if (!addr.setAddress(ip)) {
        QMessageBox::warning(this, "提示", "服务器IP格式不正确。");
        return;
    }

    const quint16 port = static_cast<quint16>(ui->portSpinBox->value());

    ui->clientStatusLabel->setText(QString("状态：连接中...（%1:%2）").arg(ip).arg(port));
    ui->connectButton->setEnabled(false);

    m_client->connectToServer(addr, port);
}

void ChatExchangeWindow::onDisconnectClicked()
{
    if (!m_client) return;
    m_client->disconnectFromHost();
}

void ChatExchangeWindow::onSendClicked()
{
    if (!m_client || !m_client->isConnected()) {
        QMessageBox::warning(this, "提示", "请先连接服务器。");
        return;
    }

    const QString msg = ui->messageLineEdit->text();
    if (msg.trimmed().isEmpty()) return;

    m_client->sendMessage(msg, QStringLiteral("message"));
    ui->messageLineEdit->clear();
    ui->messageLineEdit->setFocus();
}

void ChatExchangeWindow::onClientConnected()
{
    setClientUiConnected(true);

    const QString ip = ui->serverIpLineEdit->text().trimmed();
    const quint16 port = static_cast<quint16>(ui->portSpinBox->value());
    ui->clientStatusLabel->setText(QString("状态：已连接（%1:%2）").arg(ip).arg(port));

    // 发送登录消息：text=用户名
    m_client->sendMessage(m_username, QStringLiteral("login"));

    appendSystemMessage(QStringLiteral("已连接服务器，欢迎 %1").arg(m_username));
}

void ChatExchangeWindow::onClientDisconnected()
{
    setClientUiConnected(false);
    ui->clientStatusLabel->setText("状态：未连接");

    ui->userListWidget->clear();
    setCurrentPeer(QStringLiteral("群聊"));
    refreshOnlineCount();
    appendSystemMessage(QStringLiteral("连接已断开"));
}

void ChatExchangeWindow::onClientError(const QString& err)
{
    ui->clientStatusLabel->setText(QString("状态：错误（%1）").arg(err));
    ui->connectButton->setEnabled(true);
    appendSystemMessage(QStringLiteral("Socket错误：%1").arg(err));
}

void ChatExchangeWindow::onJsonReceived(const QJsonObject& obj)
{
    const QString type = obj.value(QStringLiteral("type")).toString();

    if (type.compare(QStringLiteral("message"), Qt::CaseInsensitive) == 0) {
        const QString sender = obj.value(QStringLiteral("sender")).toString();
        const QString text = obj.value(QStringLiteral("text")).toString();

        appendChatMessage(sender, text, sender == m_username);
        return;
    }

    if (type.compare(QStringLiteral("newUser"), Qt::CaseInsensitive) == 0) {
        const QString userName = obj.value(QStringLiteral("userName")).toString();
        if (!userName.isEmpty()) {
            addUserToList(userName);
            refreshOnlineCount();
            if (userName == m_username) {
                appendSystemMessage(QStringLiteral("你已加入聊天室"));
            } else {
                appendSystemMessage(QStringLiteral("%1 加入聊天室").arg(userName));
            }
        }
        return;
    }

    if (type.compare(QStringLiteral("userDisconnected"), Qt::CaseInsensitive) == 0) {
        const QString userName = obj.value(QStringLiteral("userName")).toString();
        if (!userName.isEmpty()) {
            removeUserFromList(userName);
            refreshOnlineCount();
            appendSystemMessage(QStringLiteral("%1 已离开").arg(userName));
        }
        return;
    }

    if (type.compare(QStringLiteral("userList"), Qt::CaseInsensitive) == 0) {
        const QJsonValue listVal = obj.value(QStringLiteral("userList"));
        if (!listVal.isArray()) return;

        ui->userListWidget->clear();
        const QJsonArray arr = listVal.toArray();
        for (const auto& v : arr) {
            if (!v.isString()) continue;
            addUserToList(v.toString());
        }
        refreshOnlineCount();
        return;
    }
}

void ChatExchangeWindow::onServerLog(const QString& msg)
{
    appendServerLogLine(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), msg));
}

void ChatExchangeWindow::setClientUiConnected(bool connected)
{
    ui->connectButton->setEnabled(!connected);
    ui->disconnectButton->setEnabled(connected);
    ui->sendButton->setEnabled(connected);
    if (connected) {
        ui->messageLineEdit->setFocus();
    }
}

void ChatExchangeWindow::appendSystemMessage(const QString& text)
{
    const QString safe = text.toHtmlEscaped();
    const QString html = QString(
        "<div style='margin:10px 0; text-align:center; color:#8b8b8b; font-size:12px;'>"
        "<span style='background:#ffffff; border:1px solid #f1e6d8; border-radius:10px; padding:4px 10px;'>%1</span>"
        "</div>").arg(safe);
    ui->chatTextEdit->append(html);
    ui->chatTextEdit->verticalScrollBar()->setValue(ui->chatTextEdit->verticalScrollBar()->maximum());
}

void ChatExchangeWindow::appendChatMessage(const QString& sender, const QString& text, bool isSelf)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm");
    const QString safeText = text.toHtmlEscaped();
    const QString safeSender = sender.toHtmlEscaped();

    QString nameLine;
    if (isSelf) {
        nameLine = QStringLiteral("%1 (我) · %2").arg(m_username.toHtmlEscaped(), time);
    } else {
        nameLine = QStringLiteral("%1 · %2").arg(safeSender, time);
    }

    // 温馨气泡：我（右侧暖黄），对方（左侧浅绿）+ 头像（圆形/首字母）
    QString html;
    if (isSelf) {
        const QString av = avatarHtml(m_username, 32);
        html = QString(
            "<div style='margin:10px 0; text-align:right;'>"
            "  <div style='display:inline-block; max-width:92%;'>"
            "    <div style='display:inline-block; max-width:72%; vertical-align:top; text-align:left;'>"
            "      <div style='font-size:12px; color:#9b8a7a; margin:0 6px 3px 0; text-align:right;'>%1</div>"
            "      <div style='background:#FFE6A7; border:1px solid #f0d08b; padding:8px 10px; border-radius:14px; line-height:1.45;'>%2</div>"
            "    </div>"
            "    <span style='display:inline-block; width:8px;'></span>"
            "    <div style='display:inline-block; vertical-align:top;'>%3</div>"
            "  </div>"
            "</div>").arg(nameLine, safeText, av);
    } else {
        const QString av = avatarHtml(sender, 32);
        html = QString(
            "<div style='margin:10px 0; text-align:left;'>"
            "  <div style='display:inline-block; max-width:92%;'>"
            "    <div style='display:inline-block; vertical-align:top;'>%3</div>"
            "    <span style='display:inline-block; width:8px;'></span>"
            "    <div style='display:inline-block; max-width:72%; vertical-align:top;'>"
            "      <div style='font-size:12px; color:#9b8a7a; margin:0 0 3px 6px;'>%1</div>"
            "      <div style='background:#E6F4EA; border:1px solid #bfe3c7; padding:8px 10px; border-radius:14px; line-height:1.45;'>%2</div>"
            "    </div>"
            "  </div>"
            "</div>").arg(nameLine, safeText, av);
    }

    ui->chatTextEdit->append(html);
    ui->chatTextEdit->verticalScrollBar()->setValue(ui->chatTextEdit->verticalScrollBar()->maximum());
}

void ChatExchangeWindow::refreshOnlineCount()
{
    const int n = ui->userListWidget->count();
    ui->onlineCountLabel->setText(QStringLiteral("在线：%1").arg(n));
}

void ChatExchangeWindow::addUserToList(const QString& userName)
{
    const QString raw = userName.trimmed();
    if (raw.isEmpty()) return;

    for (int i = 0; i < ui->userListWidget->count(); ++i) {
        auto* it = ui->userListWidget->item(i);
        if (!it) continue;
        if (it->data(Qt::UserRole).toString() == raw) {
            // 已存在：更新头像 & 如果是自己，确保有（我）标记
            it->setIcon(avatarIcon(raw, 28));
            if (raw == m_username && !it->text().contains(QStringLiteral("(我)"))) {
                it->setText(raw + QStringLiteral(" (我)"));
                QFont f = it->font();
                f.setBold(true);
                it->setFont(f);
            }
            return;
        }
    }

    auto* item = new QListWidgetItem;
    item->setIcon(avatarIcon(raw, 28));
    item->setData(Qt::UserRole, raw);
    if (raw == m_username) {
        item->setText(raw + QStringLiteral(" (我)"));
        QFont f = item->font();
        f.setBold(true);
        item->setFont(f);
    } else {
        item->setText(raw);
    }
    ui->userListWidget->addItem(item);
}

void ChatExchangeWindow::removeUserFromList(const QString& userName)
{
    const QString raw = userName.trimmed();
    if (raw.isEmpty()) return;

    for (int i = ui->userListWidget->count() - 1; i >= 0; --i) {
        auto* it = ui->userListWidget->item(i);
        if (!it) continue;
        if (it->data(Qt::UserRole).toString() == raw) {
            delete ui->userListWidget->takeItem(i);
        }
    }

    // 如果当前选中用户离开，则回到群聊
    if (m_currentPeer == raw) {
        setCurrentPeer(QStringLiteral("群聊"));
    }
}

void ChatExchangeWindow::setCurrentPeer(const QString& userName)
{
    m_currentPeer = userName;
    ui->currentPeerLabel->setText(QStringLiteral("正在与：%1").arg(m_currentPeer));
}

void ChatExchangeWindow::onUserSelectionChanged()
{
    const auto items = ui->userListWidget->selectedItems();
    if (items.isEmpty()) {
        setCurrentPeer(QStringLiteral("群聊"));
        return;
    }

    auto* it = items.first();
    const QString raw = it ? it->data(Qt::UserRole).toString() : QString();
    if (raw.isEmpty() || raw == m_username) {
        setCurrentPeer(QStringLiteral("群聊"));
    } else {
        setCurrentPeer(raw);
    }
}

void ChatExchangeWindow::appendServerLogLine(const QString& line)
{
    ui->serverLogEdit->appendPlainText(line);
}
