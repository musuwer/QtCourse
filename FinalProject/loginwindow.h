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

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void handleLogin();
    void openRegister();
    void backFromRegister();
    void otherLoginNotReady();

private:
    void initFrameless();
    void bindOtherLoginButtonsByText();

private:
    Ui::LoginWindow *ui;
    RegisterWindow* m_register = nullptr;
    MainWindow* m_main = nullptr;

    bool m_dragging = false;
    QPoint m_dragOffset;
};

#endif // LOGINWINDOW_H
