#include "idatabase.h"
#include <QUuid>
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>

// 构造函数
IDatabase::IDatabase(QObject *parent) : QObject(parent)
{
    initDatabase(); // 修正了拼写错误
}

void IDatabase::initDatabase()
{
    // 添加数据库驱动
    database = QSqlDatabase::addDatabase("QSQLITE");

    // 【建议】实际发布时建议使用 QCoreApplication::applicationDirPath() + "/Lab4.db"
    // 目前保持你原本的绝对路径
    QString aFile = "C:/Users/musuwer/Desktop/Professional_Courses/Qt/Pi_qt_project/lab3/Lab4.db";
    database.setDatabaseName(aFile);

    if(!database.open()){
        qDebug() << "failed to open database:" << database.lastError().text();
    }
    else{
        qDebug() << "ok to open database";
    }
}

QString IDatabase::userLogin(QString userName, QString userPwd)
{
    if (!database.isOpen()) {
        return "Database not open error!";
    }

    QSqlQuery query;
    // 只需要查询密码即可
    query.prepare("SELECT PASSWORD FROM User WHERE USERNAME = :USER");
    query.bindValue(":USER", userName);

    if (!query.exec()) {
        qDebug() << "Login Query Failed:" << query.lastError().text();
        return "Login Query Error";
    }

    // 【关键修复】只调用一次 first()
    if(query.first()){
        QString qpwd = query.value("PASSWORD").toString();
        // 建议去除可能存在的首尾空格
        if(qpwd.trimmed() == userPwd){
            return "Login Success!";
        }
        else{
            return "password error!";
        }
    }
    else{
        return "username error!";
    }
}

bool IDatabase::registerUser(QString userName, QString userPwd)
{
    if (userName.isEmpty() || userPwd.isEmpty()) {
        return false;
    }

    QSqlQuery query;
    // SQL语句必须包含 ID
    query.prepare("INSERT INTO User (ID, USERNAME, PASSWORD) VALUES (:id, :u, :p)");

    // 生成一个唯一的 ID (UUID)
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    query.bindValue(":id", uuid);
    query.bindValue(":u", userName);
    query.bindValue(":p", userPwd);

    bool success = query.exec();

    // 如果失败，打印具体原因
    if(!success){
        qDebug() << "Register error:" << query.lastError().text();
    }

    return success;
}

bool IDatabase::initPatientModel()
{
    if (!database.isOpen()) return false;

    patientTabModel = new QSqlTableModel(this, database);
    patientTabModel->setTable("Patient");

    // 数据保存方式：手动提交（需要调用 submitAll 才会写入数据库）
    patientTabModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    // 【修复点】Patient表通常按姓名(NAME)排序，而不是 USERNAME
    // 确保你的 Patient 表里有 NAME 列
    patientTabModel->setSort(patientTabModel->fieldIndex("NAME"), Qt::AscendingOrder);

    if(!(patientTabModel->select())){
        qDebug() << "PatientModel Select Failed:" << patientTabModel->lastError().text();
        return false;
    }

    patientSelection = new QItemSelectionModel(patientTabModel);
    return true;
}

bool IDatabase::searchPatient(QString filter)
{
    if (!patientTabModel) return false;
    patientTabModel->setFilter(filter);
    return patientTabModel->select();
}

bool IDatabase::deleteCurrentPatient()
{
    if (!patientSelection || !patientTabModel) return false;

    QModelIndex curIndex = patientSelection->currentIndex();

    // 【有效性检查】防止未选中时崩溃
    if (!curIndex.isValid()) {
        return false;
    }

    patientTabModel->removeRow(curIndex.row());

    // 提交更改
    bool success = patientTabModel->submitAll();
    if (!success) {
        qDebug() << "Delete failed:" << patientTabModel->lastError().text();
        // 如果提交失败，撤销删除操作以保持视图一致
        patientTabModel->revertAll();
    } else {
        // 刷新模型
        patientTabModel->select();
    }

    return success;
}

bool IDatabase::submitPatientEdit()
{
    if (!patientTabModel) return false;

    bool ret = patientTabModel->submitAll();
    if (!ret) {
        qDebug() << "Submit failed:" << patientTabModel->lastError().text();
    }
    return ret;
}

bool IDatabase::revertPatientEdit()
{
    if (!patientTabModel) return false;
    patientTabModel->revertAll();
    return true;
}

int IDatabase::addNewPatient()
{
    if (!patientTabModel) return -1;

    // 在末尾添加一条记录
    int row = patientTabModel->rowCount();
    patientTabModel->insertRow(row);

    // 获取刚插入的空记录
    QSqlRecord curRec = patientTabModel->record(row);

    // 设置自动生成的字段
    curRec.setValue("CREATEDTIMESTAMP", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    curRec.setValue("ID", QUuid::createUuid().toString(QUuid::WithoutBraces));

    // 将记录写回 Model
    patientTabModel->setRecord(row, curRec);

    // 返回新行的行号
    return row;
}
