#include "doctorView.h"
#include "ui_doctorView.h"

DoctorView::DoctorView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DoctorView)
{
    ui->setupUi(this);
}

DoctorView::~DoctorView()
{
    delete ui;
}
