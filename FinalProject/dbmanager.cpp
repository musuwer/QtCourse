#include "dbmanager.h"

#include <QDir>
#include <QDateTime>
#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>

DbManager& DbManager::instance()
{
    static DbManager inst;
    return inst;
}

DbManager::DbManager() {}
DbManager::~DbManager()
{
    if (m_db.isOpen()) m_db.close();
}

void DbManager::setErr(QString* errOut, const QString& msg) const
{
    if (errOut) *errOut = msg;
}

bool DbManager::init()
{
    if (m_db.isOpen()) return true;

    // 使用“当前工作目录”下的 database.db
    const QString dbPath = QDir::current().absoluteFilePath("database.db");
    qDebug() << "[DB] open:" << dbPath;

    if (QSqlDatabase::contains("project_db_conn")) {
        m_db = QSqlDatabase::database("project_db_conn");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "project_db_conn");
        m_db.setDatabaseName(dbPath);
    }

    if (!m_db.open()) {
        qDebug() << "[DB] open failed:" << m_db.lastError().text();
        return false;
    }

    return createTables();
}

bool DbManager::isOpen() const
{
    return m_db.isOpen();
}

QSqlDatabase& DbManager::database()
{
    return m_db;
}

const QSqlDatabase& DbManager::database() const
{
    return m_db;
}

bool DbManager::createTables()
{
    if (!m_db.isOpen()) return false;
    QSqlQuery q(m_db);

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS users(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            pass_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'user',
            created_at TEXT NOT NULL
        );
    )")) return false;

    // logs：兼容 4字段 or 5字段；content 允许为空
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS logs(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name TEXT,
            type TEXT,
            location TEXT,
            time TEXT,
            content TEXT,
            score INTEGER DEFAULT 0,
            created_at TEXT NOT NULL
        );
    )")) return false;

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS goals(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name TEXT,
            plan TEXT,
            progress TEXT,
            start TEXT,
            deadline TEXT,
            created_at TEXT NOT NULL
        );
    )")) return false;

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS achievements(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            f1 TEXT,
            f2 TEXT,
            f3 TEXT,
            f4 TEXT,
            f5 TEXT,
            f6 TEXT,
            created_at TEXT NOT NULL
        );
    )")) return false;

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS announcements(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            author TEXT NOT NULL,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            created_at TEXT NOT NULL
        );
    )")) return false;

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS messages(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            target TEXT NOT NULL DEFAULT '[ALL]',
            content TEXT NOT NULL,
            reply TEXT,
            created_at TEXT NOT NULL,
            replied_at TEXT
        );
    )")) return false;

    return true;
}

QByteArray DbManager::randomSalt(int len) const
{
    QByteArray s;
    s.resize(len);
    for (int i = 0; i < len; ++i) {
        s[i] = static_cast<char>(QRandomGenerator::global()->bounded(0, 256));
    }
    return s;
}

QString DbManager::hashPassword(const QString& password, const QByteArray& salt) const
{
    QByteArray in = salt + password.toUtf8();
    QByteArray h = QCryptographicHash::hash(in, QCryptographicHash::Sha256);
    return QString::fromLatin1(h.toHex());
}

// ================= 用户 =================

bool DbManager::registerUser(const QString& username, const QString& password, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }
    const QString u = username.trimmed();
    if (u.isEmpty() || password.isEmpty()) { setErr(errOut, "用户名或密码为空"); return false; }

    // 首个用户 -> admin
    QString role = "user";
    {
        QSqlQuery cnt(m_db);
        if (cnt.exec("SELECT COUNT(*) FROM users;") && cnt.next()) {
            if (cnt.value(0).toInt() == 0) role = "admin";
        }
    }

    {
        QSqlQuery ex(m_db);
        ex.prepare("SELECT 1 FROM users WHERE username=? LIMIT 1");
        ex.addBindValue(u);
        if (ex.exec() && ex.next()) { setErr(errOut, "用户名已存在"); return false; }
    }

    const QByteArray salt = randomSalt();
    const QString saltHex = QString::fromLatin1(salt.toHex());
    const QString hashHex = hashPassword(password, salt);

    QSqlQuery ins(m_db);
    ins.prepare(R"(
        INSERT INTO users(username, pass_hash, salt, role, created_at)
        VALUES(?,?,?,?,?)
    )");
    ins.addBindValue(u);
    ins.addBindValue(hashHex);
    ins.addBindValue(saltHex);
    ins.addBindValue(role);
    ins.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!ins.exec()) { setErr(errOut, ins.lastError().text()); return false; }
    return true;
}

bool DbManager::loginUser(const QString& username, const QString& password, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }
    const QString u = username.trimmed();
    if (u.isEmpty() || password.isEmpty()) { setErr(errOut, "用户名或密码为空"); return false; }

    QSqlQuery q(m_db);
    q.prepare("SELECT pass_hash, salt FROM users WHERE username=? LIMIT 1");
    q.addBindValue(u);
    if (!q.exec() || !q.next()) { setErr(errOut, "用户不存在"); return false; }

    const QString dbHash = q.value(0).toString();
    const QByteArray salt = QByteArray::fromHex(q.value(1).toByteArray());

    const QString nowHash = hashPassword(password, salt);
    if (nowHash != dbHash) { setErr(errOut, "密码错误"); return false; }
    return true;
}

int DbManager::getUserId(const QString& username, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return -1; }

    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM users WHERE username=? LIMIT 1");
    q.addBindValue(username.trimmed());
    if (!q.exec() || !q.next()) { setErr(errOut, "未找到用户"); return -1; }
    return q.value(0).toInt();
}

QString DbManager::getUserRole(const QString& username, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return "user"; }

    QSqlQuery q(m_db);
    q.prepare("SELECT role FROM users WHERE username=? LIMIT 1");
    q.addBindValue(username.trimmed());
    if (!q.exec() || !q.next()) { setErr(errOut, "未找到用户"); return "user"; }
    return q.value(0).toString();
}

// ================= 日志 =================

// 8参版本：含 content
bool DbManager::addLog(int userId,
                       const QString& name,
                       const QString& type,
                       const QString& location,
                       const QString& time,
                       const QString& content,
                       int score,
                       QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery ins(m_db);
    ins.prepare(R"(
        INSERT INTO logs(user_id, name, type, location, time, content, score, created_at)
        VALUES(?,?,?,?,?,?,?,?)
    )");
    ins.addBindValue(userId);
    ins.addBindValue(name);
    ins.addBindValue(type);
    ins.addBindValue(location);
    ins.addBindValue(time);
    ins.addBindValue(content);
    ins.addBindValue(score);
    ins.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!ins.exec()) { setErr(errOut, ins.lastError().text()); return false; }
    return true;
}

// 7参版本：不含 content（你 logrecordwindow.cpp 就会匹配这个）
bool DbManager::addLog(int userId,
                       const QString& s1,
                       const QString& s2,
                       const QString& s3,
                       const QString& s4,
                       int score,
                       QString* errOut)
{
    // 映射到 name/type/location/time，content 置空
    return addLog(userId, s1, s2, s3, s4, QString(), score, errOut);
}

bool DbManager::deleteLogById(int logId, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM logs WHERE id=?");
    q.addBindValue(logId);
    if (!q.exec()) { setErr(errOut, q.lastError().text()); return false; }
    return true;
}

// ================= 目标 =================

bool DbManager::addGoal(int userId,
                        const QString& name,
                        const QString& plan,
                        const QString& progress,
                        const QString& start,
                        const QString& deadline,
                        QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery ins(m_db);
    ins.prepare(R"(
        INSERT INTO goals(user_id, name, plan, progress, start, deadline, created_at)
        VALUES(?,?,?,?,?,?,?)
    )");
    ins.addBindValue(userId);
    ins.addBindValue(name);
    ins.addBindValue(plan);
    ins.addBindValue(progress);
    ins.addBindValue(start);
    ins.addBindValue(deadline);
    ins.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!ins.exec()) { setErr(errOut, ins.lastError().text()); return false; }
    return true;
}

bool DbManager::deleteGoalById(int goalId, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM goals WHERE id=?");
    q.addBindValue(goalId);
    if (!q.exec()) { setErr(errOut, q.lastError().text()); return false; }
    return true;
}

// ================= 公告 =================

bool DbManager::addAnnouncement(const QString& author,
                                const QString& title,
                                const QString& content,
                                QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery ins(m_db);
    ins.prepare(R"(
        INSERT INTO announcements(author, title, content, created_at)
        VALUES(?,?,?,?)
    )");
    ins.addBindValue(author);
    ins.addBindValue(title);
    ins.addBindValue(content);
    ins.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!ins.exec()) { setErr(errOut, ins.lastError().text()); return false; }
    return true;
}

bool DbManager::deleteAnnouncementById(int annId, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM announcements WHERE id=?");
    q.addBindValue(annId);
    if (!q.exec()) { setErr(errOut, q.lastError().text()); return false; }
    return true;
}

// ================= 消息 =================

bool DbManager::addMessage(const QString& username,
                           const QString& target,
                           const QString& content,
                           QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery ins(m_db);
    ins.prepare(R"(
        INSERT INTO messages(username, target, content, created_at)
        VALUES(?,?,?,?)
    )");
    ins.addBindValue(username);
    ins.addBindValue(target);
    ins.addBindValue(content);
    ins.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!ins.exec()) { setErr(errOut, ins.lastError().text()); return false; }
    return true;
}

bool DbManager::replyMessageById(int messageId,
                                 const QString& reply,
                                 QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery q(m_db);
    q.prepare("UPDATE messages SET reply=?, replied_at=? WHERE id=?");
    q.addBindValue(reply);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.addBindValue(messageId);

    if (!q.exec()) { setErr(errOut, q.lastError().text()); return false; }
    return true;
}

// ================= 成就 =================

bool DbManager::addAchievement(int userId,
                               const QString& f1,
                               const QString& f2,
                               const QString& f3,
                               const QString& f4,
                               const QString& f5,
                               const QString& f6,
                               QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery ins(m_db);
    ins.prepare(R"(
        INSERT INTO achievements(user_id, f1, f2, f3, f4, f5, f6, created_at)
        VALUES(?,?,?,?,?,?,?,?)
    )");
    ins.addBindValue(userId);
    ins.addBindValue(f1);
    ins.addBindValue(f2);
    ins.addBindValue(f3);
    ins.addBindValue(f4);
    ins.addBindValue(f5);
    ins.addBindValue(f6);
    ins.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!ins.exec()) { setErr(errOut, ins.lastError().text()); return false; }
    return true;
}

bool DbManager::deleteAchievementById(int id, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM achievements WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) { setErr(errOut, q.lastError().text()); return false; }
    return true;
}
