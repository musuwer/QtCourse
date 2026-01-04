#include "dbmanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QVariant>
#include <QDebug>

DbManager& DbManager::instance()
{
    static DbManager inst;
    return inst;
}

DbManager::DbManager()
{
    // 选择一个可写位置保存数据库（跨平台）
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    const QString dbPath = dirPath + "/trip_memory.db";
    qDebug() << "[DbManager] dbPath=" << dbPath;

    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
        m_db.setDatabaseName(dbPath);
    }

    if (!m_db.open()) {
        qWarning() << "[DbManager] open failed:" << m_db.lastError().text();
        return;
    }

    // 初始化表
    createTables();
}

DbManager::~DbManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DbManager::isOpen() const
{
    return m_db.isOpen();
}

QSqlDatabase DbManager::database() const
{
    return m_db;
}

QString DbManager::nowIso() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

bool DbManager::createTables()
{
    if (!m_db.isOpen()) return false;

    QSqlQuery q(m_db);

    // users
    if (!q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'user',
            created_at TEXT NOT NULL
        )
    )SQL")) {
        qWarning() << q.lastError();
        return false;
    }

    // logs（事件记录）
    if (!q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            person TEXT,
            place TEXT,
            log_date TEXT,
            mood_score INTEGER DEFAULT 0,
            created_at TEXT NOT NULL
        )
    )SQL")) {
        qWarning() << q.lastError();
        return false;
    }

    // goals
    if (!q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS goals (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            goal TEXT NOT NULL,
            plan TEXT,
            progress TEXT,
            start_time TEXT,
            end_time TEXT,
            created_at TEXT NOT NULL
        )
    )SQL")) {
        qWarning() << q.lastError();
        return false;
    }

    // announcements
    if (!q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS announcements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            author TEXT,
            created_at TEXT NOT NULL
        )
    )SQL")) {
        qWarning() << q.lastError();
        return false;
    }

    // achievements
    if (!q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS achievements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            type TEXT,
            level TEXT,
            org TEXT,
            ach_date TEXT,
            description TEXT,
            created_at TEXT NOT NULL
        )
    )SQL")) {
        qWarning() << q.lastError();
        return false;
    }

    // messages
    if (!q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender TEXT NOT NULL,
            receiver TEXT NOT NULL,
            message TEXT NOT NULL,
            send_time TEXT NOT NULL,
            replied INTEGER NOT NULL DEFAULT 0,
            reply TEXT,
            reply_time TEXT
        )
    )SQL")) {
        qWarning() << q.lastError();
        return false;
    }

    return true;
}

bool DbManager::registerUser(const QString& username, const QString& password, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    if (username.trimmed().isEmpty() || password.isEmpty()) { if (errMsg) *errMsg = "用户名或密码为空"; return false; }

    // 判断是否第一个用户：第一个默认 admin
    QString role = "user";
    {
        QSqlQuery q(m_db);
        if (q.exec("SELECT COUNT(*) FROM users") && q.next()) {
            const int cnt = q.value(0).toInt();
            if (cnt == 0) role = "admin";
        }
    }

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO users(username,password,role,created_at) VALUES(?,?,?,?)");
    q.addBindValue(username.trimmed());
    q.addBindValue(password);
    q.addBindValue(role);
    q.addBindValue(nowIso());

    if (!q.exec()) {
        if (errMsg) *errMsg = q.lastError().text();
        return false;
    }
    return true;
}

bool DbManager::loginUser(const QString& username, const QString& password, int* userIdOut, QString* roleOut, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }

    QSqlQuery q(m_db);
    q.prepare("SELECT id, role FROM users WHERE username=? AND password=?");
    q.addBindValue(username.trimmed());
    q.addBindValue(password);

    if (!q.exec()) {
        if (errMsg) *errMsg = q.lastError().text();
        return false;
    }
    if (!q.next()) {
        if (errMsg) *errMsg = "账号或密码错误";
        return false;
    }

    if (userIdOut) *userIdOut = q.value(0).toInt();
    if (roleOut) *roleOut = q.value(1).toString();
    return true;
}

int DbManager::getUserId(const QString& username) const
{
    if (!m_db.isOpen()) return -1;
    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM users WHERE username=?");
    q.addBindValue(username.trimmed());
    if (!q.exec()) return -1;
    if (!q.next()) return -1;
    return q.value(0).toInt();
}

QString DbManager::getUserRole(const QString& username) const
{
    if (!m_db.isOpen()) return {};
    QSqlQuery q(m_db);
    q.prepare("SELECT role FROM users WHERE username=?");
    q.addBindValue(username.trimmed());
    if (!q.exec()) return {};
    if (!q.next()) return {};
    return q.value(0).toString();
}

bool DbManager::addLog(int userId, const QString& title, const QString& person, const QString& place, const QString& dateStr, int moodScore, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    if (userId <= 0) { if (errMsg) *errMsg = "userId无效"; return false; }
    if (title.trimmed().isEmpty()) { if (errMsg) *errMsg = "标题不能为空"; return false; }

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO logs(user_id,title,person,place,log_date,mood_score,created_at) VALUES(?,?,?,?,?,?,?)");
    q.addBindValue(userId);
    q.addBindValue(title.trimmed());
    q.addBindValue(person.trimmed());
    q.addBindValue(place.trimmed());
    q.addBindValue(dateStr.trimmed());
    q.addBindValue(moodScore);
    q.addBindValue(nowIso());

    if (!q.exec()) {
        if (errMsg) *errMsg = q.lastError().text();
        return false;
    }
    return true;
}

bool DbManager::deleteLogById(int logId, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM logs WHERE id=?");
    q.addBindValue(logId);
    if (!q.exec()) { if (errMsg) *errMsg = q.lastError().text(); return false; }
    return true;
}

bool DbManager::addGoal(int userId, const QString& goal, const QString& plan, const QString& progress, const QString& startDate, const QString& endDate, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    if (userId <= 0) { if (errMsg) *errMsg = "userId无效"; return false; }
    if (goal.trimmed().isEmpty()) { if (errMsg) *errMsg = "目标不能为空"; return false; }

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO goals(user_id,goal,plan,progress,start_time,end_time,created_at) VALUES(?,?,?,?,?,?,?)");
    q.addBindValue(userId);
    q.addBindValue(goal.trimmed());
    q.addBindValue(plan.trimmed());
    q.addBindValue(progress.trimmed());
    q.addBindValue(startDate.trimmed());
    q.addBindValue(endDate.trimmed());
    q.addBindValue(nowIso());

    if (!q.exec()) { if (errMsg) *errMsg = q.lastError().text(); return false; }
    return true;
}

bool DbManager::deleteGoalById(int goalId, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM goals WHERE id=?");
    q.addBindValue(goalId);
    if (!q.exec()) { if (errMsg) *errMsg = q.lastError().text(); return false; }
    return true;
}

bool DbManager::addAnnouncement(const QString& title, const QString& content, const QString& author, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    if (title.trimmed().isEmpty() || content.trimmed().isEmpty()) { if (errMsg) *errMsg = "标题或内容为空"; return false; }

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO announcements(title,content,author,created_at) VALUES(?,?,?,?)");
    q.addBindValue(title.trimmed());
    q.addBindValue(content.trimmed());
    q.addBindValue(author.trimmed());
    q.addBindValue(nowIso());

    if (!q.exec()) { if (errMsg) *errMsg = q.lastError().text(); return false; }
    return true;
}

bool DbManager::deleteAnnouncementById(int annId, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM announcements WHERE id=?");
    q.addBindValue(annId);
    if (!q.exec()) { if (errMsg) *errMsg = q.lastError().text(); return false; }
    return true;
}

bool DbManager::addAchievement(int userId, const QString& name, const QString& type, const QString& level, const QString& org, const QString& dateStr, const QString& desc, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    if (userId <= 0) { if (errMsg) *errMsg = "userId无效"; return false; }
    if (name.trimmed().isEmpty()) { if (errMsg) *errMsg = "成就名称不能为空"; return false; }

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO achievements(user_id,name,type,level,org,ach_date,description,created_at) VALUES(?,?,?,?,?,?,?,?)");
    q.addBindValue(userId);
    q.addBindValue(name.trimmed());
    q.addBindValue(type.trimmed());
    q.addBindValue(level.trimmed());
    q.addBindValue(org.trimmed());
    q.addBindValue(dateStr.trimmed());
    q.addBindValue(desc.trimmed());
    q.addBindValue(nowIso());

    if (!q.exec()) { if (errMsg) *errMsg = q.lastError().text(); return false; }
    return true;
}

bool DbManager::deleteAchievementById(int achId, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM achievements WHERE id=?");
    q.addBindValue(achId);
    if (!q.exec()) { if (errMsg) *errMsg = q.lastError().text(); return false; }
    return true;
}

bool DbManager::addMessage(const QString& sender, const QString& receiver, const QString& message, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    if (sender.trimmed().isEmpty() || receiver.trimmed().isEmpty() || message.trimmed().isEmpty()) {
        if (errMsg) *errMsg = "sender/receiver/message 不能为空";
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO messages(sender,receiver,message,send_time,replied) VALUES(?,?,?,?,0)");
    q.addBindValue(sender.trimmed());
    q.addBindValue(receiver.trimmed());
    q.addBindValue(message.trimmed());
    q.addBindValue(nowIso());

    if (!q.exec()) { if (errMsg) *errMsg = q.lastError().text(); return false; }
    return true;
}

bool DbManager::replyMessageById(int msgId, const QString& reply, QString* errMsg)
{
    if (!m_db.isOpen()) { if (errMsg) *errMsg = "数据库未打开"; return false; }
    if (reply.trimmed().isEmpty()) { if (errMsg) *errMsg = "回复不能为空"; return false; }

    QSqlQuery q(m_db);
    q.prepare("UPDATE messages SET replied=1, reply=?, reply_time=? WHERE id=?");
    q.addBindValue(reply.trimmed());
    q.addBindValue(nowIso());
    q.addBindValue(msgId);

    if (!q.exec()) { if (errMsg) *errMsg = q.lastError().text(); return false; }
    return true;
}
