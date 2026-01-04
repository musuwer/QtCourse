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

private:
    void initUi();
    void connectSignals();

private slots:
    void onLoginClicked();
    void onRegisterClicked();

    void onMinClicked();
    void onMaxClicked();
    void onCloseClicked();

    void onRegisterBack();
    void onRegisterSuccess(const QString& username);

private:
    Ui::LoginWindow *ui;
    RegisterWindow* m_registerWindow = nullptr;
    MainWindow* m_mainWindow = nullptr;
};

#endif // LOGINWINDOW_H
