#include "dbmanager.h"

#include <QDir>
#include <QDateTime>
#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>
#include <QCoreApplication> // 用于获取应用程序路径

/**
 * @brief 获取 DbManager 的单例对象
 * @return DbManager& 引用
 */
DbManager& DbManager::instance()
{
    static DbManager inst;
    return inst;
}

DbManager::DbManager() {}

DbManager::~DbManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

/**
 * @brief 辅助函数：设置错误信息
 * @param errOut 错误信息输出指针
 * @param msg 错误内容
 */
void DbManager::setErr(QString* errOut, const QString& msg) const
{
    if (errOut) {
        *errOut = msg;
        qWarning() << "[DbManager Error]" << msg; // 同时也打印到控制台，方便调试
    }
}

/**
 * @brief 初始化数据库连接并创建表结构
 * @return true 成功, false 失败
 */
bool DbManager::init()
{
    // 如果数据库已经打开，直接返回成功
    if (m_db.isOpen()) return true;

    // 1. 确定数据库文件路径
    // 使用应用程序所在目录，确保在不同电脑上都能找到并读写
    const QString dbPath = QCoreApplication::applicationDirPath() + "/mood_tracker.db";
    qDebug() << "[DB] Database path:" << dbPath;

    // 2. 建立数据库连接
    // 检查是否已经存在连接，避免 "Duplicate connection name" 错误
    if (QSqlDatabase::contains("project_db_conn")) {
        m_db = QSqlDatabase::database("project_db_conn");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "project_db_conn");
        m_db.setDatabaseName(dbPath);
    }

    // 3. 打开数据库
    if (!m_db.open()) {
        qCritical() << "[DB] Open failed:" << m_db.lastError().text();
        return false;
    }

    // 4. 创建所需的表结构
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

/**
 * @brief 创建所有必要的数据库表
 * 使用 "IF NOT EXISTS" 避免重复创建报错
 */
bool DbManager::createTables()
{
    if (!m_db.isOpen()) return false;
    QSqlQuery q(m_db);

    // --- 1. 用户表 (users) ---
    // 存储用户名、密码哈希、盐值、角色
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

    // --- 2. 日志表 (logs) ---
    // 存储心情日志、活动、地点、时间等
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

    // --- 3. 目标表 (goals) ---
    // 存储个人目标、计划、进度
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

    // --- 4. 成就表 (achievements) ---
    // 存储获奖、证书等信息 (f1-f6 对应不同字段)
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

    // --- 5. 公告表 (announcements) ---
    // 管理员发布的通知
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS announcements(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            author TEXT NOT NULL,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            created_at TEXT NOT NULL
        );
    )")) return false;

    // --- 6. 消息/反馈表 (messages) ---
    // 用户反馈和管理员回复
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

// ================= 安全相关 (密码哈希) =================

/**
 * @brief 生成随机盐值
 * @param len 盐值长度
 * @return 二进制盐值
 */
QByteArray DbManager::randomSalt(int len) const
{
    QByteArray s;
    s.resize(len);
    for (int i = 0; i < len; ++i) {
        // 生成 0-255 之间的随机字节
        s[i] = static_cast<char>(QRandomGenerator::global()->bounded(0, 256));
    }
    return s;
}

/**
 * @brief 计算密码哈希 (SHA-256)
 * @param password 原始密码
 * @param salt 盐值
 * @return 十六进制哈希字符串
 */
QString DbManager::hashPassword(const QString& password, const QByteArray& salt) const
{
    // 将盐值和密码拼接后进行哈希，极大增加破解难度
    QByteArray in = salt + password.toUtf8();
    QByteArray h = QCryptographicHash::hash(in, QCryptographicHash::Sha256);
    return QString::fromLatin1(h.toHex());
}

// ================= 用户管理 =================

bool DbManager::registerUser(const QString& username, const QString& password, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库初始化失败"); return false; }
    const QString u = username.trimmed();
    if (u.isEmpty() || password.isEmpty()) { setErr(errOut, "用户名或密码不能为空"); return false; }

    // 1. 检查是否是第一个注册的用户，如果是则自动设为管理员
    QString role = "user";
    {
        QSqlQuery cnt(m_db);
        if (cnt.exec("SELECT COUNT(*) FROM users;") && cnt.next()) {
            if (cnt.value(0).toInt() == 0) role = "admin";
        }
    }

    // 2. 检查用户名是否已存在
    {
        QSqlQuery ex(m_db);
        ex.prepare("SELECT 1 FROM users WHERE username=? LIMIT 1");
        ex.addBindValue(u);
        if (ex.exec() && ex.next()) { setErr(errOut, "该用户名已被注册"); return false; }
    }

    // 3. 生成密码哈希
    const QByteArray salt = randomSalt();
    const QString saltHex = QString::fromLatin1(salt.toHex());
    const QString hashHex = hashPassword(password, salt);

    // 4. 插入数据库
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

    if (!ins.exec()) { setErr(errOut, "注册失败: " + ins.lastError().text()); return false; }
    return true;
}

bool DbManager::loginUser(const QString& username, const QString& password, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库初始化失败"); return false; }
    const QString u = username.trimmed();
    if (u.isEmpty() || password.isEmpty()) { setErr(errOut, "用户名或密码为空"); return false; }

    // 1. 获取该用户的盐值和哈希密码
    QSqlQuery q(m_db);
    q.prepare("SELECT pass_hash, salt FROM users WHERE username=? LIMIT 1");
    q.addBindValue(u);
    if (!q.exec() || !q.next()) { setErr(errOut, "用户不存在"); return false; }

    const QString dbHash = q.value(0).toString();
    const QByteArray salt = QByteArray::fromHex(q.value(1).toByteArray());

    // 2. 使用相同的盐值计算输入密码的哈希，并比对
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
    if (!q.exec() || !q.next()) { setErr(errOut, "未找到该用户ID"); return -1; }
    return q.value(0).toInt();
}

QString DbManager::getUserRole(const QString& username, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return "user"; }

    QSqlQuery q(m_db);
    q.prepare("SELECT role FROM users WHERE username=? LIMIT 1");
    q.addBindValue(username.trimmed());
    if (!q.exec() || !q.next()) { setErr(errOut, "未找到该用户角色"); return "user"; }
    return q.value(0).toString();
}

// ================= 日志管理 (Logs) =================

// 完整版：包含 content 字段
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

    if (!ins.exec()) { setErr(errOut, "添加日志失败: " + ins.lastError().text()); return false; }
    return true;
}

// 简化版：不含 content (用于旧接口兼容)
bool DbManager::addLog(int userId,
                       const QString& s1,
                       const QString& s2,
                       const QString& s3,
                       const QString& s4,
                       int score,
                       QString* errOut)
{
    // 调用完整版，将 content 设为空字符串
    return addLog(userId, s1, s2, s3, s4, QString(), score, errOut);
}

bool DbManager::deleteLogById(int logId, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM logs WHERE id=?");
    q.addBindValue(logId);
    if (!q.exec()) { setErr(errOut, "删除日志失败: " + q.lastError().text()); return false; }
    return true;
}

// ================= 目标管理 (Goals) =================

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

    if (!ins.exec()) { setErr(errOut, "添加目标失败: " + ins.lastError().text()); return false; }
    return true;
}

bool DbManager::deleteGoalById(int goalId, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM goals WHERE id=?");
    q.addBindValue(goalId);
    if (!q.exec()) { setErr(errOut, "删除目标失败: " + q.lastError().text()); return false; }
    return true;
}

// ================= 公告管理 (Announcements) =================

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

    if (!ins.exec()) { setErr(errOut, "发布公告失败: " + ins.lastError().text()); return false; }
    return true;
}

bool DbManager::deleteAnnouncementById(int annId, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM announcements WHERE id=?");
    q.addBindValue(annId);
    if (!q.exec()) { setErr(errOut, "删除公告失败: " + q.lastError().text()); return false; }
    return true;
}

// ================= 消息反馈 (Messages) =================

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

    if (!ins.exec()) { setErr(errOut, "发送消息失败: " + ins.lastError().text()); return false; }
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

    if (!q.exec()) { setErr(errOut, "回复消息失败: " + q.lastError().text()); return false; }
    return true;
}

// ================= 成就管理 (Achievements) =================

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

    if (!ins.exec()) { setErr(errOut, "添加成就失败: " + ins.lastError().text()); return false; }
    return true;
}

bool DbManager::deleteAchievementById(int id, QString* errOut)
{
    if (!init()) { setErr(errOut, "数据库未打开"); return false; }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM achievements WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) { setErr(errOut, "删除成就失败: " + q.lastError().text()); return false; }
    return true;
}
