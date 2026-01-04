#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QWidget>

namespace Ui {
class RegisterWindow;
}

class RegisterWindow : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    ~RegisterWindow();

signals:
    void backToLoginRequested();

private slots:
    void handleRegister();
    void handleReturn();

private:
    Ui::RegisterWindow *ui;
};

#endif // REGISTERWINDOW_H
