#include "replacedialog.h"
#include "ui_replacedialog.h"
#include <QDebug>
#include <QMessageBox>

replaceDialog::replaceDialog(QWidget *parent,QPlainTextEdit * textEdit) :
    QDialog(parent),
    ui(new Ui::replaceDialog)
{
    ui->setupUi(this);

    pTextEdit = textEdit;
}

replaceDialog::~replaceDialog()
{
    delete ui;
}

void replaceDialog::on_findNextButton_clicked()
{

    int index = -1;

    //目标串
    QString target = ui->searchText->text();

    if(target.isEmpty()){
        QMessageBox::warning(this,"替换","请输入查找内容！");
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
            QMessageBox::warning(this,"替换","未找到 "+target);
        }
        else{
            QMessageBox::warning(this,"替换","已是最后一个 "+target);
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


void replaceDialog::on_replaceButton_clicked()
{
    on_findNextButton_clicked();

    //目标串
    QString target = ui->searchText->text();
    //替换掉内容
    QString to = ui->targetText->text();

    //选中、插入
    QString selText = pTextEdit->textCursor().selectedText();
    if(selText == target){
        pTextEdit->insertPlainText(to);
    }

}


void replaceDialog::on_replaceAllButton_clicked()
{
    QString target  = ui->searchText->text();
    QString to = ui->targetText->text();
    if((pTextEdit != nullptr) && (!target.isEmpty()) && (!to.isEmpty())){
        QString text = pTextEdit->toPlainText();

        text.replace(target,to,ui->checkBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

        pTextEdit->clear();
        pTextEdit->insertPlainText(text);
    }
}


void replaceDialog::on_cancelButton_clicked()
{
    accept();
}

