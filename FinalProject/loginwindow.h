#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QPoint>

#ifdef Q_OS_WIN
#include <QByteArray>
#endif

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
    bool eventFilter(QObject *obj, QEvent *event) override;

#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private slots:
    void handleLogin();
    void openRegister();
    void backFromRegister();
    void otherLoginNotReady();

private:
    void bindOtherLoginButtonsByText();

    void setupFrameless();
    void bindWindowButtons();

private:
    Ui::LoginWindow *ui;
    RegisterWindow* m_register = nullptr;
    MainWindow* m_main = nullptr;

    // frameless drag
    bool m_dragging = false;
    QPoint m_dragPos;
};

#endif // LOGINWINDOW_H
