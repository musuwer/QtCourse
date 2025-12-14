#include "patienteditview.h"
#include "ui_patienteditview.h"
#include "idatabase.h"
#include <QSqlTableModel>

patientEditView::patientEditView(QWidget *parent,int rowNum) :
    QWidget(parent),
    ui(new Ui::patientEditView)
{
    ui->setupUi(this);
    dataMapper = new QDataWidgetMapper();
    QSqlTableModel *tabModel = IDatabase::getInstance().patientTabModel;
    dataMapper->setModel(IDatabase::getInstance().patientTabModel);
    dataMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

    dataMapper->addMapping(ui->lineEditID_2,tabModel->fieldIndex("ID"));
    dataMapper->addMapping(ui->NameInput,tabModel->fieldIndex("NAME"));
    dataMapper->addMapping(ui->IDCardInput,tabModel->fieldIndex("ID_CARD"));
    dataMapper->addMapping(ui->HeightDoubleSpinBox,tabModel->fieldIndex("HEIGHT"));
    dataMapper->addMapping(ui->WeightDoubleSpinBox,tabModel->fieldIndex("WEIGHT"));
    dataMapper->addMapping(ui->MobilePhoneInput,tabModel->fieldIndex("MOBILEPHONE"));
    dataMapper->addMapping(ui->BirthDateBox,tabModel->fieldIndex("DOB"));
    dataMapper->addMapping(ui->SexComboBox,tabModel->fieldIndex("SEX"));
    dataMapper->addMapping(ui->CreateTimeStamp,tabModel->fieldIndex("CREATEDTIMESTAMP"));
    dataMapper->addMapping(ui->lineEditAge,tabModel->fieldIndex("AGE"));

    dataMapper->setCurrentIndex(rowNum);
}

patientEditView::~patientEditView()
{
    delete ui;
}

void patientEditView::on_btnSave_clicked()
{
    IDatabase::getInstance().submitPatientEdit();
    emit goPreviousView();
}


void patientEditView::on_btnCancel_clicked()
{
    IDatabase::getInstance().revertPatientEdit();
    emit goPreviousView();

}

