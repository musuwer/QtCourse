#include "idatabase.h"
#include <QUuid>

void IDatabase::ininDatabase()
{
    //添加数据库驱动
    database = QSqlDatabase::addDatabase("QSQLITE");
    QString aFile = "C:/Users/musuwer/Desktop/Professional_Courses/Qt/Pi_qt_project/lab3/Lab3.db";
    database.setDatabaseName(aFile);    //设置数据库名称
    if(!database.open()){
        qDebug()<<"failed to open database";
    }
    else{
        qDebug()<<"ok to open database";
    }
}

QString IDatabase::userLogin(QString userName, QString userPwd)
{
    QSqlQuery query;
    query.prepare("SELECT USERNAME,PASSWORD FROM User WHERE USERNAME = :USER");
    query.bindValue(":USER",userName);
    query.exec();
    query.first();
    if(query.first() && query.value("USERNAME").isValid()){
        QString qpwd = query.value("PASSWORD").toString();
        if(qpwd == userPwd){
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


// idatabase.cpp
bool IDatabase::registerUser(QString userName, QString userPwd)
{
    QSqlQuery query;
    // 【修改点1】SQL语句必须包含 ID
    query.prepare("INSERT INTO User (ID, USERNAME, PASSWORD) VALUES (:id, :u, :p)");

    // 【修改点2】生成一个唯一的 ID (UUID)
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    query.bindValue(":id", uuid);

    query.bindValue(":u", userName);
    query.bindValue(":p", userPwd);

    // qDebug() << "Writing to DB file:" << QSqlDatabase::database().databaseName();
    // Writing to DB file: "C:/Users/musuwer/Desktop/Professional_Courses/Qt/Pi_qt_project/lab3/Lab4.db"

    bool success = query.exec();

    // 【调试建议】如果失败，打印具体原因
    if(!success){
        qDebug() << "Register error:" << query.lastError().text();
    }

    return success;
}
bool IDatabase::initPatientModel()
{

    patientTabModel = new QSqlTableModel(this,database);
    patientTabModel->setTable("Patient");

    patientTabModel->setEditStrategy(QSqlTableModel::OnManualSubmit);//数据保存方式
    patientTabModel->setSort(patientTabModel->fieldIndex("USERNAME"),Qt::AscendingOrder);//按 姓名 排序
    if(!(patientTabModel->select())){
        qDebug()<<"123sdds";
        return false;
    }
    patientSelection = new QItemSelectionModel(patientTabModel);
    return true;

}

bool IDatabase::searchPatient(QString filter)
{
    patientTabModel->setFilter(filter);
    return patientTabModel->select();
}

bool IDatabase::deleteCurrentPatient()
{
    QModelIndex curIndex = patientSelection->currentIndex();
    // 建议加上有效性检查
    if (!curIndex.isValid()) {
        return false;
    }

    patientTabModel->removeRow(curIndex.row());

    // 提交更改，并保存结果
    bool success = patientTabModel->submitAll();

    // 刷新模型
    patientTabModel->select();

    // 【修复点】返回执行结果
    return success;
}
bool IDatabase::submitPatientEdit()
{
    return patientTabModel->submitAll();
}

bool IDatabase::revertPatientEdit()
{
    patientTabModel->revertAll();
    // 【修复点】必须返回一个值，通常撤销操作视为成功，返回 true
    return true;
}

int IDatabase::addNewPatient(){
    //在末尾添加一条记录
    patientTabModel->insertRow(patientTabModel->rowCount(),
                               QModelIndex());
    QModelIndex curIndex = patientTabModel->index(patientTabModel->rowCount()-1,1);
    int curRecNo = curIndex.row();
    //获取当前记录
    QSqlRecord curRec = patientTabModel->record(curRecNo);
    curRec.setValue("CREATEDTIMESTAMP",QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    curRec.setValue("ID",QUuid::createUuid().toString(QUuid::WithoutBraces));
    patientTabModel->setRecord(curRecNo,curRec);
    return curIndex.row();
}

IDatabase::IDatabase(QObject *parent) : QObject(parent)
{
    ininDatabase();
}

