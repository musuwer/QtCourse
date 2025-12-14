#ifndef IDATABASE_H
#define IDATABASE_H

#include <QObject>
#include <QtSql>
#include <QSqlDatabase>
#include <QDataWidgetMapper>


//单例模式 不可以从外部直接实例化，只能用内部已经实例化好的唯一一个实例
class IDatabase : public QObject
{
    Q_OBJECT
public:

    static IDatabase &getInstance(){
        static IDatabase instance;
        return instance;
    }

    QString userLogin(QString userName,QString userPwd);

    //数据模型
    QSqlTableModel *patientTabModel;
    //选择模型
    QItemSelectionModel *patientSelection;
signals:

private:
    explicit IDatabase(QObject *parent = nullptr);
    IDatabase(IDatabase const&) = delete;
    void operator=(IDatabase const&)=delete;

    QSqlDatabase database;
    void initDatabase();

public:
    bool initPatientModel();
    bool searchPatient(QString filter);
    bool deleteCurrentPatient();
    bool submitPatientEdit();
    bool revertPatientEdit();
    bool registerUser(QString userName, QString userPwd);

    int addNewPatient();



};

#endif // IDATABASE_H
