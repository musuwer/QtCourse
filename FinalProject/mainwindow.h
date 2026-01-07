#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

namespace Ui {
class MainWindow;
}

class HomeWindow;
class AchievementWindow;
class LogRecordWindow;
class MoodCalendarWindow;
class MessageUserWindow;
class MessageAdminWindow;
class AboutWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& username, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void logoutRequested();

private:
    void initFrameless();
    void initUi();
    void initPages();
    void connectSignals();

private slots:
    void onNavChanged(int row);
    void onLogoutClicked();

    void onMinClicked();
    void onMaxClicked();
    void onCloseClicked();

private:
    Ui::MainWindow *ui;

    QString m_username;
    int m_userId = -1;
    QString m_role;

    HomeWindow* m_home = nullptr;
    AchievementWindow* m_achievement = nullptr;
    LogRecordWindow* m_log = nullptr;
    MoodCalendarWindow* m_mood = nullptr;
    QWidget* m_messagePage = nullptr; // user/admin 二选一
    AboutWindow* m_about = nullptr;

    bool m_dragging = false;
    QPoint m_dragOffset;
};

#endif // MAINWINDOW_H
