    #include "loginview.h"
#include "ui_loginview.h"
#include "idatabase.h"
#include <QMessageBox>
#include <QApplication>

LoginView::LoginView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginView)
{
    ui->setupUi(this);
}

LoginView::~LoginView()
{
    delete ui;
}

void LoginView::on_btnLogin_clicked()
{
    QString result = IDatabase::getInstance().userLogin(ui->lineEditUserName->text(),ui->lineEditPwd->text());
    qDebug()<<result;
    if(result == "Login Success!"){
        emit loginSucess();
    }
    else{
        //警铃
        QApplication::beep();
        //错误弹窗
        QMessageBox::warning(this,"登录错误",result);
    }


}

void LoginView::on_btnRegister_clicked() {
    QString name = ui->lineEditUserName->text();
    QString pwd = ui->lineEditPwd->text();

    if (IDatabase::getInstance().registerUser(name, pwd)) {
        QMessageBox::information(this, "提示", "注册成功！");
    } else {
        QMessageBox::warning(this, "错误", "注册失败");
    }
}
