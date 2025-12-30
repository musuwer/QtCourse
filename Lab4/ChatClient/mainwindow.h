#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "chatclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnLogin_clicked();

    void on_btnExit_clicked();

    void on_btnSent_clicked();

    void on_btnLogout_clicked();

    void connectedToServer();

    void messageReviced(const QString &sender, const QString &text);

    void jsonReceived(const QJsonObject &docObj);
    void userJoined(const QString &user);
    void userDelete(const QString &user);
    void userListReceived(const QStringList &list);
private:
    Ui::MainWindow *ui;
    ChatClient *m_chatClient;
};
#endif // MAINWINDOW_H
