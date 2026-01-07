#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>

#include "dbmanager.h"
#include "loginwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 影响 QStandardPaths::AppDataLocation 的路径组成
    QCoreApplication::setOrganizationName("CampusGrowth");
    QCoreApplication::setApplicationName("CampusGrowth");

    // 初始化数据库（首次运行会自动创建 db + 表）
    QString dbErr;
    if (!DbManager::instance().init(&dbErr)) {
        const QString hint = QString(
            "数据库初始化失败：\n%1\n\n"
            "当前数据库路径：\n%2\n\n"
            "排查建议：\n"
            "1) 确认 database.db 没有被 SQLiteStudio/DB Browser 等工具打开（会锁库）\n"
            "2) 确认 database.db 不是只读文件\n"
            "3) 如果你在 QtCreator 里运行：建议把 database.db 放在项目根目录（含 .pro 文件的目录）\n"
            "4) 如果仍失败：删除旧的 database.db 让程序重新创建\n"
        ).arg(dbErr, DbManager::instance().databasePath());
        QMessageBox::critical(nullptr, "CampusGrowth", hint);
        return -1;
    }

    LoginWindow w;
    w.show();

    return a.exec();
}
