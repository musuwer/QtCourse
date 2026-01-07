#ifndef HOMEWINDOW_H
#define HOMEWINDOW_H

#include <QWidget>
#include <QString>

class QPushButton;

namespace Ui {
class HomeWindow;
}

class HomeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit HomeWindow(int userId, const QString& username, const QString& role, QWidget *parent = nullptr);
    ~HomeWindow();

public slots:
    void refreshAll();

private:
    void initUi();
    void connectSignals();

    void refreshGoals();
    void refreshAnnouncements();

    void setupTableWidgets();

private slots:
    void onAddGoalClicked();
    void onAddAnnouncementClicked();
    void onRefreshGoalClicked();
    void onRefreshAnnClicked();

    void onExportGoalCsvClicked();

    void onGoalContextMenu(const QPoint& pos);
    void onAnnContextMenu(const QPoint& pos);

private:
    Ui::HomeWindow *ui;

    int m_userId = -1;
    QString m_username;
    QString m_role;

    QPushButton* m_exportGoalCsvBtn = nullptr;
};

#endif // HOMEWINDOW_H
