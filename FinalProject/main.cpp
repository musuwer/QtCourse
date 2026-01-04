#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>

#include "dbmanager.h"
#include "loginwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 影响 QStandardPaths::AppDataLocation 的路径组成，建议设置
    QCoreApplication::setOrganizationName("TripMemory");
    QCoreApplication::setApplicationName("TripMemory");

    // 初始化数据库（首次运行会自动创建 db + 表）
    if (!DbManager::instance().init()) {
        QMessageBox::critical(nullptr, "Trip Memory", "数据库初始化失败，请检查写入权限。");
        return -1;
    }

    LoginWindow w;
    w.show();

    return a.exec();
}
