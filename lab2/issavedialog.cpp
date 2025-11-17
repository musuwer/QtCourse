#include "issavedialog.h"
#include "ui_issavedialog.h"
#include "mainwindow.h"

isSaveDialog::isSaveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::isSaveDialog)
{
    ui->setupUi(this);
}

isSaveDialog::~isSaveDialog()
{
    delete ui;
}

void isSaveDialog::on_saveButton_clicked()
{

}

