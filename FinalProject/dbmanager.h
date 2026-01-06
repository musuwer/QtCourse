#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QString>
#include <QSqlDatabase>

class DbManager
{
public:
    static DbManager& instance();

    // ✅ 固定使用：exe 同目录下的 database.db（最不容易路径错）
    // errOut: 返回更详细的失败原因（用于弹窗提示）
    bool init(QString* errOut = nullptr);

    // 当前实际使用的数据库文件路径
    QString databasePath() const;
    bool isOpen() const;

    QSqlDatabase& database();
    const QSqlDatabase& database() const;

    bool createTables(QString* errOut = nullptr);

    // ===== 用户 =====
    // ✅ 明文存储（pass_hash 字段当密码字段用），login 兼容旧哈希
    // ✅ 3参版本：默认注册普通用户
bool registerUser(const QString& username, const QString& password, QString* errOut = nullptr);

// ✅ 4参版本：兼容旧代码（registerwindow.cpp 传 role）
// 注意：不要给 role 写默认参数，否则会和 (username,password,errOut) 调用产生歧义
bool registerUser(const QString& username, const QString& password, const QString& role, QString* errOut = nullptr);
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

    // 7参版本：userId + 4个QString + score + err（你的 logrecordwindow.cpp 会匹配这个）
    bool addLog(int userId,
                const QString& s1,
                const QString& s2,
                const QString& s3,
                const QString& s4,
                int score,
                QString* errOut = nullptr);

    bool deleteLogById(int logId, QString* errOut = nullptr);

    // ===== 目标 =====
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
    bool addMessage(const QString& username,
                    const QString& target,
                    const QString& content,
                    QString* errOut = nullptr);

    bool replyMessageById(int messageId,
                          const QString& reply,
                          QString* errOut = nullptr);

    // ===== 成就 =====
    bool addAchievement(int userId,
                        const QString& f1,
                        const QString& f2,
                        const QString& f3,
                        const QString& f4,
                        const QString& f5,
                        const QString& f6,
                        QString* errOut = nullptr);

    bool deleteAchievementById(int id, QString* errOut = nullptr);

    // ✅ 一键清空数据 + 重新写入默认数据（admin/admin + 示例大三用户等）
    bool resetAndSeed(QString* errOut = nullptr);

private:
    DbManager();
    ~DbManager();

    void setErr(QString* errOut, const QString& msg) const;

    // 旧哈希兼容用（如果你数据库里已有 salt 非空的账号）
    QByteArray randomSalt(int len = 16) const;
    QString hashPassword(const QString& password, const QByteArray& salt) const;

private:
    QSqlDatabase m_db;
};

#endif // DBMANAGER_H
