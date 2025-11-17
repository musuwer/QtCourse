#include "searchdialog.h"
#include "ui_searchdialog.h"
#include <QDebug>
#include <QMessageBox>


searchDialog::searchDialog(QWidget *parent, QPlainTextEdit *textEdit) :
    QDialog(parent),
    ui(new Ui::searchDialog)
{
    ui->setupUi(this);
    pTextEdit = textEdit;
}

searchDialog::~searchDialog()
{
    delete ui;
}

void searchDialog::on_findNextButton_clicked()
{
    int index = -1;

    //目标串
    QString target = ui->searchText->text();

    if(target.isEmpty()){
        QMessageBox::warning(this,"查找","请输入查找内容！");
        return;
    }

    //整个文件
    QString srcText = pTextEdit->toPlainText();

    //获取光标
    QTextCursor c = pTextEdit->textCursor();

    //如果选择了向下查找的按钮
    if(ui->raDownButton->isChecked()){
        //从光标开始往后找，前面的不找
        index = srcText.indexOf(target,c.position(),ui->checkBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

        if(index >=0){
            c.setPosition(index);
            c.setPosition(index + target.length(),QTextCursor::KeepAnchor);

            pTextEdit->setTextCursor(c);
            count++;
            qDebug()<<count;
        }
        else if(index<=0 && count ==0){
            QMessageBox::warning(this,"查找","未找到 "+target);
        }
        else{
            QMessageBox::warning(this,"查找","已是最后一个 "+target);
        }
    }

    if(ui->raUpButton->isChecked()){
        index = srcText.lastIndexOf(target,c.position() - target.length(),ui->checkBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

        if(index >=0){
            c.setPosition(index + target.length());
            c.setPosition(index,QTextCursor::KeepAnchor);

            pTextEdit->setTextCursor(c);

            count++;
        }
        else if(index<=0 && count ==0){
            QMessageBox::warning(this,"查找","未找到 "+target);
        }
        else{
            QMessageBox::warning(this,"查找","已是第一个 "+target);
        }

    }
}


void searchDialog::on_cancelButton_clicked()
{
    accept();
}

