#include "dbmanager.h"

DbManager::DbManager()
{
    // 获取标准数据存储路径，兼容不同系统
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(path);
    if (!dir.exists()) dir.mkpath(".");

    QString dbPath = path + "/mood_tracker.db";
    qDebug() << "Database path:" << dbPath;

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qDebug() << "Error: connection with database failed";
    } else {
        qDebug() << "Database: connection ok";
        createTables();
    }
}

DbManager::~DbManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

DbManager& DbManager::instance()
{
    static DbManager instance;
    return instance;
}

bool DbManager::isOpen() const
{
    return m_db.isOpen();
}

bool DbManager::createTables()
{
    QSqlQuery query;
    // 创建用户表
    bool success = query.exec("CREATE TABLE IF NOT EXISTS users ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "username TEXT UNIQUE, "
                              "password TEXT)");

    // 创建日志表 (对应 logrecord_window)
    query.exec("CREATE TABLE IF NOT EXISTS logs ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "user_id INTEGER, "
               "date TEXT, "
               "mood TEXT, "
               "content TEXT, "
               "FOREIGN KEY(user_id) REFERENCES users(id))");

    return success;
}

bool DbManager::registerUser(const QString& username, const QString& password)
{
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password) VALUES (:name, :pass)");
    query.bindValue(":name", username);
    query.bindValue(":pass", password); // 实际项目中建议加密存储
    return query.exec();
}

bool DbManager::loginUser(const QString& username, const QString& password)
{
    QSqlQuery query;
    query.prepare("SELECT id FROM users WHERE username = :name AND password = :pass");
    query.bindValue(":name", username);
    query.bindValue(":pass", password);

    if (query.exec()) {
        return query.next(); // 如果有结果则登录成功
    }
    return false;
}

int DbManager::getUserId(const QString& username) {
    QSqlQuery query;
    query.prepare("SELECT id FROM users WHERE username = :name");
    query.bindValue(":name", username);
    if(query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}
