#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QMessageBox>
#include "mainwindow.h"
#include "registerwindow.h"
#include "dbmanager.h"

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

private slots:
    void on_btnLogin_clicked();    // 假设UI里按钮叫 btnLogin
    void on_btnRegister_clicked(); // 假设UI里按钮叫 btnRegister

private:
    Ui::LoginWindow *ui;
    MainWindow *m_mainWindow;      // 主界面指针
    RegisterWindow *m_registerWindow; // 注册界面指针
};

#endif // LOGINWINDOW_H
