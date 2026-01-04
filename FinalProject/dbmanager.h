#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QString>
#include <QSqlDatabase>

class DbManager
{
public:
    static DbManager& instance();

    // 打开 ./database.db（相对运行工作目录）
    bool init();
    bool isOpen() const;

    QSqlDatabase& database();
    const QSqlDatabase& database() const;

    bool createTables();

    // ===== 用户 =====
    bool registerUser(const QString& username, const QString& password, QString* errOut = nullptr);
    bool loginUser(const QString& username, const QString& password, QString* errOut = nullptr);
    int  getUserId(const QString& username, QString* errOut = nullptr);
    QString getUserRole(const QString& username, QString* errOut = nullptr);

    // ===== 日志（事件记录）=====
    // 8参版本：userId + 5个QString + score + err
    bool addLog(int userId,
                const QString& name,
                const QString& type,
                const QString& location,
                const QString& time,
                const QString& content,
                int score,
                QString* errOut = nullptr);

    // 7参版本：userId + 4个QString + score + err（你 logrecordwindow.cpp 当前就是这种）
    bool addLog(int userId,
                const QString& s1,
                const QString& s2,
                const QString& s3,
                const QString& s4,
                int score,
                QString* errOut = nullptr);

    bool deleteLogById(int logId, QString* errOut = nullptr);

    // ===== 目标 =====
    // 7参：userId + 5个QString + err（你 homewindow.cpp 当前就是这种）
    bool addGoal(int userId,
                 const QString& name,
                 const QString& plan,
                 const QString& progress,
                 const QString& start,
                 const QString& deadline,
                 QString* errOut = nullptr);

    bool deleteGoalById(int goalId, QString* errOut = nullptr);

    // ===== 公告 =====
    bool addAnnouncement(const QString& author,
                         const QString& title,
                         const QString& content,
                         QString* errOut = nullptr);

    bool deleteAnnouncementById(int annId, QString* errOut = nullptr);

    // ===== 消息 =====
    // 4参：username + target + content + err（你 messageuserwindow.cpp 当前就是这种）
    bool addMessage(const QString& username,
                    const QString& target,
                    const QString& content,
                    QString* errOut = nullptr);

    // 3参：messageId + reply + err（你 messageadminwindow.cpp 当前就是这种）
    bool replyMessageById(int messageId,
                          const QString& reply,
                          QString* errOut = nullptr);

    // ===== 成就 =====
    // 8参：userId + 6个QString + err（你 achievementwindow.cpp 当前就是这种）
    bool addAchievement(int userId,
                        const QString& f1,
                        const QString& f2,
                        const QString& f3,
                        const QString& f4,
                        const QString& f5,
                        const QString& f6,
                        QString* errOut = nullptr);

    bool deleteAchievementById(int id, QString* errOut = nullptr);

private:
    DbManager();
    ~DbManager();

    void setErr(QString* errOut, const QString& msg) const;

    QByteArray randomSalt(int len = 16) const;
    QString hashPassword(const QString& password, const QByteArray& salt) const;

private:
    QSqlDatabase m_db;
};

#endif // DBMANAGER_H
