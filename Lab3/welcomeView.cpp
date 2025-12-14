#include "welcomeView.h"
#include "ui_welcomeView.h"

WelcomeView::WelcomeView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WelcomeView)
{
    ui->setupUi(this);
}

WelcomeView::~WelcomeView()
{
    delete ui;
}

void WelcomeView::on_btnSectionManage_clicked()
{
    emit goSectionView();
}


void WelcomeView::on_btnDoctorManage_clicked()
{
    emit goDoctorView();
}


void WelcomeView::on_btnPatientManage_clicked()
{
    emit goPatientView();
}

