#include "dbmanager.h"

#include <QDir>
#include <QDateTime>
#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>
#include <QCoreApplication>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtGlobal>
#include <QSet>

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

static bool tableHasColumn(QSqlDatabase& db, const QString& table, const QString& col)
{
    QSqlQuery q(db);
    q.exec(QString("PRAGMA table_info(%1);").arg(table));
    while (q.next()) {
        // PRAGMA table_info: cid, name, type, notnull, dflt_value, pk
        if (q.value(1).toString().compare(col, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

static QSet<QString> tableColumns(QSqlDatabase& db, const QString& table)
{
    QSet<QString> cols;
    QSqlQuery q(db);
    q.exec(QString("PRAGMA table_info(%1);").arg(table));
    while (q.next()) {
        cols.insert(q.value(1).toString().trimmed().toLower());
    }
    return cols;
}


QString DbManager::databasePath() const
{
    const QString dbName = "database.db";

    // ✅ 优先：找到 .pro 所在目录（项目根目录），直接用它同级的 database.db
    // 注意：先从 currentPath 找，再从 exeDir 找，避免 Qt Creator 工作目录就是项目目录时走偏
    const QStringList startDirs = {
        QDir::currentPath(),
        QCoreApplication::applicationDirPath()
    };

    auto findProDir = [&](const QString& start) -> QString {
        QDir d(start);
        for (int depth = 0; depth < 12; ++depth) {
            const QStringList pros = d.entryList(QStringList() << "*.pro", QDir::Files);
            if (!pros.isEmpty()) {
                return d.absolutePath(); // 这个目录就是 .pro 所在目录
            }
            if (!d.cdUp()) break;
        }
        return QString();
    };

    // 1) 只要找到 .pro 目录，就优先使用它同级的 database.db（不存在也用这个路径创建）
    for (const QString& start : startDirs) {
        const QString proDirPath = findProDir(start);
        if (!proDirPath.isEmpty()) {
            QDir proDir(proDirPath);
            return QDir::cleanPath(proDir.absoluteFilePath(dbName));
        }
    }

    // 2) 找不到 .pro：再退化为“向上查找是否存在 database.db”
    for (const QString& start : startDirs) {
        QDir d(start);
        for (int depth = 0; depth < 12; ++depth) {
            const QString candidateDb = d.absoluteFilePath(dbName);
            if (QFileInfo::exists(candidateDb)) {
                return QDir::cleanPath(candidateDb);
            }
            if (!d.cdUp()) break;
        }
    }

    // 3) 仍找不到：放在 exe 同目录
    QDir exeDir(QCoreApplication::applicationDirPath());
    return QDir::cleanPath(exeDir.absoluteFilePath(dbName));
}



bool DbManager::init(QString* errOut)
{
    if (m_db.isOpen()) return true;

    // ✅ 固定使用 exe 同目录下 database.db
    const QString dbPath = databasePath();
    qDebug() << "[DB] using:" << dbPath;

    if (QSqlDatabase::contains("project_db_conn")) {
        m_db = QSqlDatabase::database("project_db_conn");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "project_db_conn");
        m_db.setDatabaseName(dbPath);
    }

    if (!m_db.open()) {
        const QString msg = m_db.lastError().text();
        qDebug() << "[DB] open failed:" << msg;
        setErr(errOut, msg);
        return false;
    }

    if (!createTables(errOut)) return false;

    // ✅ 强制确保 admin/admin 存在且可登录（兼容不同 users 表结构）
    {
        const bool hasPassword = tableHasColumn(m_db, "users", "password");
        const bool hasPassHash = tableHasColumn(m_db, "users", "pass_hash");

        QSqlQuery q(m_db);
        if (hasPassword) {
            q.exec("INSERT OR IGNORE INTO users(username, password, role, created_at) "
                   "VALUES('admin','admin','admin', datetime('now'))");
            q.exec("UPDATE users SET password='admin', role='admin' WHERE username='admin'");
        } else if (hasPassHash) {
            q.exec("INSERT OR IGNORE INTO users(username, pass_hash, salt, role, created_at) "
                   "VALUES('admin','admin','', 'admin', datetime('now'))");
            q.exec("UPDATE users SET pass_hash='admin', salt='', role='admin' WHERE username='admin'");
        } else {
            // 极端情况：users 表不是预期结构
            qDebug() << "[DB] users table has unknown schema; cannot ensure admin account.";
        }
    }

    return true;
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

bool DbManager::createTables(QString* errOut)
{
    if (!m_db.isOpen()) {
        setErr(errOut, "数据库未打开");
        return false;
    }

    QSqlQuery q(m_db);
    auto execSql = [&](const QString& sql) -> bool {
        if (!q.exec(sql)) {
            setErr(errOut, q.lastError().text());
            return false;
        }
        return true;
    };

    // ------------------ users ------------------
    if (!execSql(R"SQL(
        CREATE TABLE IF NOT EXISTS users(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'user',
            created_at TEXT DEFAULT (datetime('now'))
        );
    )SQL")) return false;

    // ------------------ logs (migrate if needed) ------------------
    auto ensureLogsTable = [&]() -> bool {
        // If not exists, create the expected schema.
        if (!m_db.tables().contains("logs")) {
            return execSql(R"SQL(
                CREATE TABLE IF NOT EXISTS logs(
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    user_id INTEGER NOT NULL,
                    title TEXT,
                    person TEXT,
                    place TEXT,
                    log_date TEXT,
                    mood_score INTEGER DEFAULT 0,
                    created_at TEXT NOT NULL DEFAULT (datetime('now'))
                );
            )SQL");
        }

        const QSet<QString> cols = tableColumns(m_db, "logs");
        const bool schemaOk = cols.contains("title") && cols.contains("person") && cols.contains("place")
                              && cols.contains("log_date") && cols.contains("mood_score") && cols.contains("created_at");
        if (schemaOk) return true;

        // Rename old table, create new table, then copy best-effort.
        const QString backup = QString("logs_backup_%1").arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
        if (!execSql(QString("ALTER TABLE logs RENAME TO %1;").arg(backup))) return false;

        if (!execSql(R"SQL(
            CREATE TABLE logs(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                title TEXT,
                person TEXT,
                place TEXT,
                log_date TEXT,
                mood_score INTEGER DEFAULT 0,
                created_at TEXT NOT NULL DEFAULT (datetime('now'))
            );
        )SQL")) return false;

        const QString titleExpr   = cols.contains("title")      ? "title"      : (cols.contains("name")     ? "name"     : "''");
        const QString personExpr  = cols.contains("person")     ? "person"     : (cols.contains("type")     ? "type"     : "''");
        const QString placeExpr   = cols.contains("place")      ? "place"      : (cols.contains("location") ? "location" : "''");
        const QString dateExpr    = cols.contains("log_date")   ? "log_date"   : (cols.contains("time")     ? "time"     : "''");
        const QString moodExpr    = cols.contains("mood_score") ? "mood_score" : (cols.contains("score")    ? "score"    : "0");
        const QString createdExpr = cols.contains("created_at") ? "created_at" : "datetime('now')";

        const QString copySql = QString(
            "INSERT INTO logs(user_id,title,person,place,log_date,mood_score,created_at) "
            "SELECT user_id, %1, %2, %3, %4, %5, %6 FROM %7;"
        ).arg(titleExpr, personExpr, placeExpr, dateExpr, moodExpr, createdExpr, backup);

        if (!execSql(copySql)) return false;
        return true;
    };

    if (!ensureLogsTable()) return false;

    // ------------------ goals ------------------
    if (!execSql(R"SQL(
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
    )SQL")) return false;

    // ------------------ achievements ------------------
    if (!execSql(R"SQL(
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
    )SQL")) return false;

    // ------------------ announcements ------------------
    if (!execSql(R"SQL(
        CREATE TABLE IF NOT EXISTS announcements(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            author TEXT NOT NULL,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            created_at TEXT NOT NULL
        );
    )SQL")) return false;

    // ------------------ messages ------------------
    if (!execSql(R"SQL(
        CREATE TABLE IF NOT EXISTS messages(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            target TEXT NOT NULL DEFAULT '[ALL]',
            content TEXT NOT NULL,
            reply TEXT,
            created_at TEXT NOT NULL,
            replied_at TEXT
        );
    )SQL")) return false;

    // ------------------ mood_records ------------------
    if (!execSql(R"SQL(
        CREATE TABLE IF NOT EXISTS mood_records(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            mood_date TEXT NOT NULL,
            mood_score INTEGER NOT NULL DEFAULT 0,
            mood_text TEXT,
            updated_at TEXT NOT NULL DEFAULT (datetime('now')),
            UNIQUE(user_id, mood_date)
        );
    )SQL")) return false;

    // Seed mood_records from logs (best-effort) if empty.
    {
        QSqlQuery c(m_db);
        if (c.exec("SELECT COUNT(*) FROM mood_records") && c.next()) {
            const int cnt = c.value(0).toInt();
            if (cnt == 0) {
                execSql(R"SQL(
                    INSERT OR IGNORE INTO mood_records(user_id,mood_date,mood_score,mood_text,updated_at)
                    SELECT user_id,
                           substr(log_date,1,10) as mood_date,
                           CAST(ROUND(AVG(mood_score)) AS INTEGER) as mood_score,
                           '',
                           datetime('now')
                    FROM logs
                    WHERE log_date IS NOT NULL AND log_date <> ''
                    GROUP BY user_id, substr(log_date,1,10);
                )SQL");

                execSql(R"SQL(
                    UPDATE mood_records
                    SET mood_text = CASE
                        WHEN mood_score <= 2 THEN '难过'
                        WHEN mood_score <= 4 THEN '一般'
                        WHEN mood_score <= 6 THEN '平静'
                        WHEN mood_score <= 8 THEN '开心'
                        ELSE '兴奋'
                    END
                    WHERE mood_text IS NULL OR mood_text='';
                )SQL");
            }
        }
    }

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

// ================= 用户（明文存储 + 兼容旧哈希）=================

bool DbManager::registerUser(const QString& username, const QString& password, QString* errOut)
{
    // 3参版本：默认注册为普通用户（兼容旧调用）
    return registerUser(username, password, QString("user"), errOut);
}

bool DbManager::registerUser(const QString& username, const QString& password, const QString& role, QString* errOut)
{
    if (!init(errOut)) return false;

    const QString u = username.trimmed();
    if (u.isEmpty() || password.isEmpty()) { setErr(errOut, "用户名或密码为空"); return false; }

    // ✅ 注册页传入的 role 只允许为 user（避免用户自行注册管理员）
    QString r = role.trimmed().toLower();
    if (r != "user") r = "user";

    // 重名检查
    {
        QSqlQuery ex(m_db);
        ex.prepare("SELECT 1 FROM users WHERE username=? LIMIT 1");
        ex.addBindValue(u);
        if (!ex.exec()) { setErr(errOut, ex.lastError().text()); return false; }
        if (ex.next()) { setErr(errOut, "用户名已存在"); return false; }
    }

    // ✅ 数据集兼容：优先写入 users.password；否则写入 pass_hash/salt（明文）
    const bool hasPassword = tableHasColumn(m_db, "users", "password");
    const bool hasPassHash = tableHasColumn(m_db, "users", "pass_hash");

    QSqlQuery ins(m_db);
    if (hasPassword) {
        ins.prepare(R"(
            INSERT INTO users(username, password, role, created_at)
            VALUES(?,?,?,?)
        )");
        ins.addBindValue(u);
        ins.addBindValue(password);
        ins.addBindValue(r);
        ins.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    } else if (hasPassHash) {
        ins.prepare(R"(
            INSERT INTO users(username, pass_hash, salt, role, created_at)
            VALUES(?,?,?,?,?)
        )");
        ins.addBindValue(u);
        ins.addBindValue(password);
        ins.addBindValue(QString(""));
        ins.addBindValue(r);
        ins.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    } else {
        setErr(errOut, "users 表结构异常：缺少 password/pass_hash 字段");
        return false;
    }


    if (!ins.exec()) { setErr(errOut, ins.lastError().text()); return false; }
    return true;
}

bool DbManager::loginUser(const QString& username, const QString& password, QString* errOut)
{
    if (!init(errOut)) return false;

    const QString u = username.trimmed();
    const QString p = password; // 不强制 trimmed，避免用户密码包含空格（一般不会）
    if (u.isEmpty() || p.isEmpty()) { setErr(errOut, "用户名或密码为空"); return false; }

    const bool hasPassword = tableHasColumn(m_db, "users", "password");
    const bool hasPassHash = tableHasColumn(m_db, "users", "pass_hash");

    // ✅ 数据集格式：users.password
    if (hasPassword) {
        QSqlQuery q(m_db);
        q.prepare("SELECT password FROM users WHERE username=? LIMIT 1");
        q.addBindValue(u);
        if (!q.exec()) { setErr(errOut, q.lastError().text()); return false; }
        if (!q.next()) { setErr(errOut, "用户不存在"); return false; }

        const QString stored = q.value(0).toString();
        if (stored == p) return true;

        setErr(errOut, "密码错误");
        return false;
    }

    // ✅ 兼容旧库：users.pass_hash + users.salt（salt 为空视为明文）
    if (hasPassHash) {
        QSqlQuery q(m_db);
        q.prepare("SELECT pass_hash, salt FROM users WHERE username=? LIMIT 1");
        q.addBindValue(u);
        if (!q.exec()) { setErr(errOut, q.lastError().text()); return false; }
        if (!q.next()) { setErr(errOut, "用户不存在"); return false; }

        const QString stored = q.value(0).toString();
        const QString saltHex = q.value(1).toString();

        if (saltHex.trimmed().isEmpty()) {
            if (stored == p) return true;
            setErr(errOut, "密码错误");
            return false;
        }

        const QByteArray salt = QByteArray::fromHex(saltHex.toLatin1());
        const QString nowHash = hashPassword(p, salt);
        if (nowHash == stored) return true;

        setErr(errOut, "密码错误");
        return false;
    }

    setErr(errOut, "users 表结构异常：缺少 password/pass_hash 字段");
    return false;
}

int DbManager::getUserId(const QString& username, QString* errOut)
{
    if (!init(errOut)) return -1;

    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM users WHERE username=? LIMIT 1");
    q.addBindValue(username.trimmed());
    if (!q.exec() || !q.next()) { setErr(errOut, "未找到用户"); return -1; }
    return q.value(0).toInt();
}

QString DbManager::getUserRole(const QString& username, QString* errOut)
{
    if (!init(errOut)) return "user";

    QSqlQuery q(m_db);
    q.prepare("SELECT role FROM users WHERE username=? LIMIT 1");
    q.addBindValue(username.trimmed());
    if (!q.exec() || !q.next()) { setErr(errOut, "未找到用户"); return "user"; }

    // ✅ 统一角色字符串，避免出现 "Admin"、"admin "、"管理员" 导致判断失败
    const QString raw = q.value(0).toString();
    QString r = raw.trimmed();
    if (r.isEmpty()) return "user";

    const QString lower = r.toLower();
    // 常见管理员写法统一映射为 "admin"
    if (lower == "admin" || lower == "administrator" || r == QStringLiteral("管理员")) {
        return "admin";
    }

    // 默认：非管理员一律视为普通用户（避免未知 role 导致权限穿透）
    return "user";
}

// ================= 日志 =================

bool DbManager::addLog(int userId,
                       const QString& title,
                       const QString& person,
                       const QString& place,
                       const QString& logDate,
                       const QString& note,
                       int moodScore,
                       QString* errOut)
{
    if (!init(errOut)) return false;

    // logs: id,user_id,title,person,place,log_date,mood_score,created_at
    // note 字段目前不落库（兼容旧调用）
    Q_UNUSED(note);

    QSqlQuery ins(m_db);
    ins.prepare(R"(
        INSERT INTO logs(user_id, title, person, place, log_date, mood_score, created_at)
        VALUES(?,?,?,?,?,?,?)
    )");
    ins.addBindValue(userId);
    ins.addBindValue(title);
    ins.addBindValue(person);
    ins.addBindValue(place);
    ins.addBindValue(logDate);
    ins.addBindValue(moodScore);
    ins.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!ins.exec()) { setErr(errOut, ins.lastError().text()); return false; }
    return true;
}


bool DbManager::addLog(int userId,
                       const QString& title,
                       const QString& person,
                       const QString& place,
                       const QString& logDate,
                       int moodScore,
                       QString* errOut)
{
    return addLog(userId, title, person, place, logDate, QString(), moodScore, errOut);
}

bool DbManager::deleteLogById(int logId, QString* errOut)
{
    if (!init(errOut)) return false;

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
    if (!init(errOut)) return false;

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
    if (!init(errOut)) return false;

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
    if (!init(errOut)) return false;

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
    if (!init(errOut)) return false;

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
    if (!init(errOut)) return false;

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
    if (!init(errOut)) return false;

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
    if (!init(errOut)) return false;

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
    if (!init(errOut)) return false;

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM achievements WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) { setErr(errOut, q.lastError().text()); return false; }
    return true;
}

// ================= 一键清空并重置默认数据 =================

bool DbManager::resetAndSeed(QString* errOut)
{
    if (!init(errOut)) return false;

    QSqlQuery q(m_db);
    if (!q.exec("DELETE FROM messages;")) { setErr(errOut, q.lastError().text()); return false; }
    if (!q.exec("DELETE FROM announcements;")) { setErr(errOut, q.lastError().text()); return false; }
    if (!q.exec("DELETE FROM achievements;")) { setErr(errOut, q.lastError().text()); return false; }
    if (!q.exec("DELETE FROM goals;")) { setErr(errOut, q.lastError().text()); return false; }
    if (!q.exec("DELETE FROM logs;")) { setErr(errOut, q.lastError().text()); return false; }
    // mood_records 可能还没有数据：一并清空
    q.exec("DELETE FROM mood_records;");
    if (!q.exec("DELETE FROM users;")) { setErr(errOut, q.lastError().text()); return false; }

    const bool hasPassword = tableHasColumn(m_db, "users", "password");
    const bool hasPassHash = tableHasColumn(m_db, "users", "pass_hash");

    // admin/admin
    if (hasPassword) {
        if (!q.exec("INSERT INTO users(username, password, role, created_at) "
                    "VALUES('admin','admin', 'admin', datetime('now'))")) {
            setErr(errOut, q.lastError().text());
            return false;
        }
    } else if (hasPassHash) {
        if (!q.exec("INSERT INTO users(username, pass_hash, salt, role, created_at) "
                    "VALUES('admin','admin','', 'admin', datetime('now'))")) {
            setErr(errOut, q.lastError().text());
            return false;
        }
    } else {
        setErr(errOut, "users 表结构异常：缺少 password/pass_hash 字段");
        return false;
    }

    // 示例用户：大三学生
    if (hasPassword) {
        if (!q.exec("INSERT INTO users(username, password, role, created_at) "
                    "VALUES('chenyu','123456', 'user', datetime('now'))")) {
            setErr(errOut, q.lastError().text());
            return false;
        }
    } else {
        if (!q.exec("INSERT INTO users(username, pass_hash, salt, role, created_at) "
                    "VALUES('chenyu','123456','', 'user', datetime('now'))")) {
            setErr(errOut, q.lastError().text());
            return false;
        }
    }

    // 公告
    q.exec("INSERT INTO announcements(author,title,content,created_at) VALUES('admin','系统使用说明','账号：admin/admin。示例学生：chenyu/123456。可以添加日志、目标、成就、留言。', datetime('now'))");
    q.exec("INSERT INTO announcements(author,title,content,created_at) VALUES('admin','本周提醒','记得完成Qt课程设计报告、整理截图并录屏演示。', datetime('now'))");
    q.exec("INSERT INTO announcements(author,title,content,created_at) VALUES('admin','假期计划','每天至少学习2小时：Qt+数据库+项目文档。', datetime('now'))");

    // 目标
    const int uid = getUserId("chenyu");
    {
        QSqlQuery ins(m_db);
        ins.prepare("INSERT INTO goals(user_id,name,plan,progress,start,deadline,created_at) VALUES(?,?,?,?,?,?,datetime('now'))");
        ins.addBindValue(uid);
        ins.addBindValue("Qt课程设计冲刺");
        ins.addBindValue("完善登录/注册/数据库联动；完成主页面数据展示；录屏演示");
        ins.addBindValue("进行中 60%");
        ins.addBindValue("2026-01-01");
        ins.addBindValue("2026-01-08");
        ins.exec();
    }

    // 日志
    {
        addLog(uid, "图书馆自习", "学习", "图书馆三楼", "2026-01-03 19:00", "复习Qt信号槽和SQLite接口。", 4, nullptr);
        addLog(uid, "小组讨论", "项目", "线上会议", "2026-01-02 21:00", "确认UI控件命名与数据库字段映射。", 5, nullptr);
    }

    // 将日志的日期+评分映射到 mood_records（按天聚合为 1 条）
    q.exec(R"SQL(
        INSERT OR IGNORE INTO mood_records(user_id,mood_date,mood_score,mood_text,updated_at)
        SELECT user_id,
               substr(log_date,1,10) as mood_date,
               CAST(ROUND(AVG(mood_score)) AS INTEGER) as mood_score,
               '',
               datetime('now')
        FROM logs
        WHERE log_date IS NOT NULL AND log_date <> ''
        GROUP BY user_id, substr(log_date,1,10);
    )SQL");

    q.exec(R"SQL(
        UPDATE mood_records
        SET mood_text = CASE
            WHEN mood_score <= 2 THEN '难过'
            WHEN mood_score <= 4 THEN '一般'
            WHEN mood_score <= 6 THEN '平静'
            WHEN mood_score <= 8 THEN '开心'
            ELSE '兴奋'
        END
        WHERE mood_text IS NULL OR mood_text='';
    )SQL");

    // 成就
    {
        QSqlQuery ins(m_db);
        ins.prepare("INSERT INTO achievements(user_id,f1,f2,f3,f4,f5,f6,created_at) VALUES(?,?,?,?,?,?,?,datetime('now'))");
        ins.addBindValue(uid);
        ins.addBindValue("完成登录注册流程");
        ins.addBindValue("数据库可持久化");
        ins.addBindValue("公告/目标/日志数据可展示");
        ins.addBindValue("完成一次演示录屏");
        ins.addBindValue("完成报告初稿");
        ins.addBindValue("完成最终提交");
        ins.exec();
    }

    // 留言 + 管理员回复
    q.exec("INSERT INTO messages(username,target,content,reply,created_at,replied_at) VALUES('chenyu','[ALL]','管理员你好，登录注册功能我已经调通了，后续准备完善日志页面。','收到，注意控件名和ui一致，最后记得Clean+qmake再打包提交。', datetime('now'), datetime('now'))");

    return true;
}
