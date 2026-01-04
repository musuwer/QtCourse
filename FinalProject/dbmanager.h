#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QSqlDatabase>
#include <QString>

/**
 * DbManager：SQLite 数据库管理（单例）
 * - 打开数据库（存放于 AppDataLocation）
 * - 初始化表结构
 * - 提供项目基础 CRUD 接口
 */
class DbManager
{
public:
    static DbManager& instance();

    bool isOpen() const;
    QSqlDatabase database() const;

    bool createTables();

    // 用户
    bool registerUser(const QString& username, const QString& password, QString* errMsg = nullptr);
    bool loginUser(const QString& username, const QString& password, int* userIdOut = nullptr, QString* roleOut = nullptr, QString* errMsg = nullptr);
    int getUserId(const QString& username) const;
    QString getUserRole(const QString& username) const;

    // 日志（事件记录）
    bool addLog(int userId,
                const QString& title,
                const QString& person,
                const QString& place,
                const QString& dateStr,
                int moodScore,
                QString* errMsg = nullptr);

    bool deleteLogById(int logId, QString* errMsg = nullptr);

    // 目标（主页）
    bool addGoal(int userId,
                 const QString& goal,
                 const QString& plan,
                 const QString& progress,
                 const QString& startDate,
                 const QString& endDate,
                 QString* errMsg = nullptr);

    bool deleteGoalById(int goalId, QString* errMsg = nullptr);

    // 公告（主页）
    bool addAnnouncement(const QString& title,
                         const QString& content,
                         const QString& author,
                         QString* errMsg = nullptr);

    bool deleteAnnouncementById(int annId, QString* errMsg = nullptr);

    // 成就
    bool addAchievement(int userId,
                        const QString& name,
                        const QString& type,
                        const QString& level,
                        const QString& org,
                        const QString& dateStr,
                        const QString& desc,
                        QString* errMsg = nullptr);

    bool deleteAchievementById(int achId, QString* errMsg = nullptr);

    // 消息（反馈交流）
    bool addMessage(const QString& sender,
                    const QString& receiver,
                    const QString& message,
                    QString* errMsg = nullptr);

    bool replyMessageById(int msgId,
                          const QString& reply,
                          QString* errMsg = nullptr);

private:
    DbManager();
    ~DbManager();

    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    QString nowIso() const;

private:
    QSqlDatabase m_db;
};

#endif // DBMANAGER_H
