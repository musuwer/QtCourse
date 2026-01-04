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
    void requestBack();
    void registerSuccess(const QString& username);

private:
    void initUi();
    void connectSignals();

private slots:
    void onRegisterClicked();
    void onReturnClicked();

    void onMinClicked();
    void onMaxClicked();
    void onCloseClicked();

private:
    Ui::RegisterWindow *ui;
};

#endif // REGISTERWINDOW_H
