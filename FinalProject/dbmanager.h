#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

class DbManager
{
public:
    static DbManager& instance(); // 单例模式

    bool isOpen() const;
    bool createTables(); // 初始化表结构

    // 用户相关功能
    bool registerUser(const QString& username, const QString& password);
    bool loginUser(const QString& username, const QString& password);
    int getUserId(const QString& username);

private:
    DbManager(); // 私有构造
    ~DbManager();
    QSqlDatabase m_db;
};

#endif // DBMANAGER_H
