#include "patientmanage.h"
#include "ui_patientmanage.h"
#include "idatabase.h"

PatientManage::PatientManage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PatientManage)
{
    ui->setupUi(this);

    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setAlternatingRowColors(true);

    IDatabase &iDatabase = IDatabase::getInstance();
    if(iDatabase.initPatientModel()){
        qDebug()<<"123";
        ui->tableView->setModel(iDatabase.patientTabModel);
        ui->tableView->setSelectionModel(iDatabase.patientSelection);
    }

}

PatientManage::~PatientManage()
{
    delete ui;
}

void PatientManage::on_btnSearch_clicked()
{
    QString filter = QString("NAME like '%%1%'").arg(ui->lineEdit->text());
    IDatabase::getInstance().searchPatient(filter);
}


void PatientManage::on_btnAdd_clicked()
{
    int currow = IDatabase::getInstance().addNewPatient();
    emit goPatientEditView(currow);
}


void PatientManage::on_btnDelete_clicked()
{
    IDatabase::getInstance().deleteCurrentPatient();
}

void PatientManage::on_btnUpdate_clicked()
{
    QModelIndex curIndex =
        IDatabase::getInstance().patientSelection->currentIndex();//获取当前选择单元格的模型索引；
    emit goPatientEditView(curIndex.row());
}

