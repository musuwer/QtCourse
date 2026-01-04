#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>

namespace Ui {
class LoginWindow;
}

class RegisterWindow;
class MainWindow;

class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

private slots:
    void handleLogin();
    void openRegister();
    void backFromRegister();
    void otherLoginNotReady();

private:
    void bindOtherLoginButtonsByText();

private:
    Ui::LoginWindow *ui;
    RegisterWindow* m_register = nullptr;
    MainWindow* m_main = nullptr;
};

#endif // LOGINWINDOW_H
