#include "masterview.h"
#include "ui_masterview.h"
#include "idatabase.h"
#include <QDebug>

MasterView::MasterView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MasterView)
{
    ui->setupUi(this);

    //    this->setWindowFlag(Qt::FramelessWindowHint);

    goLoginView();


    //实例化数据库
    IDatabase::getInstance();
}

MasterView::~MasterView()
{
    delete ui;
}

void MasterView::goLoginView()
{
    loginView = new LoginView(this);
    pushWidgetTOStackView(loginView);

    connect(loginView,SIGNAL(loginSucess()),this,SLOT(goWelcomView()));
}

void MasterView::goWelcomView()
{
    welcomView = new WelcomeView(this);
    pushWidgetTOStackView(welcomView);

    connect(welcomView,SIGNAL(goDoctorView()),this,SLOT(goDoctorView()));
    connect(welcomView,SIGNAL(goSectionView()),this,SLOT(goSectionView()));
    connect(welcomView,SIGNAL(goPatientView()),this,SLOT(goPatientView()));
}

void MasterView::goDoctorView()
{
    doctorView = new DoctorView(this);
    pushWidgetTOStackView(doctorView);
}

void MasterView::goSectionView()
{
    sectionView = new SectionView(this);
    pushWidgetTOStackView(sectionView);
}

void MasterView::goPatientView()
{
    patientView = new PatientManage(this);
    pushWidgetTOStackView(patientView);

    connect(patientView,SIGNAL(goPatientEditView(int)),this,SLOT(goPatientEditView(int)));
}

void MasterView::goPatientEditView(int rowNum)
{
    patientEditView1 = new patientEditView(this,rowNum);
    pushWidgetTOStackView(patientEditView1);

    connect(patientEditView1,SIGNAL(goPreviousView()),this,SLOT(goPreviousView()));
}

void MasterView::goPreviousView()
{
    int count = ui->stackedWidget->count();
    if(count > 1){
        //切换页面
        ui->stackedWidget->setCurrentIndex(count-2);
        ui->labelTitle->setText(ui->stackedWidget->currentWidget()->windowTitle());

        //获取旧页面、移除页面、删除页面
        QWidget * widget = ui->stackedWidget->widget(count-1);
        ui->stackedWidget->removeWidget(widget);
        delete widget;
    }
}

void MasterView::pushWidgetTOStackView(QWidget *widget)
{
    ui->stackedWidget->addWidget(widget);
    int count = ui->stackedWidget->count();
    ui->stackedWidget->setCurrentIndex(count-1);
    ui->labelTitle->setText(widget->windowTitle());
}


void MasterView::on_btnBack_clicked()
{
    goPreviousView();
}


void MasterView::on_btnLogout_clicked()
{
    int count = ui->stackedWidget->count();
    for(count;count>1;count--){
        QWidget *widget = ui->stackedWidget->widget(count-1);
        ui->stackedWidget->removeWidget(widget);
        delete  widget;
    }
    goLoginView();

    //删除注销前的页面，只剩下loginView
    QWidget *widget = ui->stackedWidget->widget(count-1);
    ui->stackedWidget->removeWidget(widget);
    delete  widget;
}


void MasterView::on_stackedWidget_currentChanged(int arg1)
{
    int count = ui->stackedWidget->count();
    if(count>1){
        ui->btnBack->setEnabled(true);
    }
    else {
        ui->btnBack->setEnabled(false);
    }
    QString title = ui->stackedWidget->currentWidget()->windowTitle();
    if(title == "欢迎"){
        ui->btnBack->setEnabled(false);
        ui->btnLogout->setEnabled(true);
    }
    if(title == "登录"){
        ui->btnBack->setEnabled(false);
        ui->btnLogout->setEnabled(false);
    }

}

