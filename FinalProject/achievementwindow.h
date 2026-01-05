#ifndef ACHIEVEMENTWINDOW_H
#define ACHIEVEMENTWINDOW_H

#include <QWidget>
#include <QString>

namespace Ui {
class AchievementWindow;
}

class AchievementWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AchievementWindow(int userId, const QString& username, const QString& role, QWidget *parent = nullptr);
    ~AchievementWindow();

public slots:
    void refreshData();

private:
    void initUi();
    void connectSignals();

    void doSearch();

private slots:
    void onAddClicked();
    void onRefreshClicked();
    void onSearchClicked();
    void onTableContextMenu(const QPoint& pos);

private:
    Ui::AchievementWindow *ui;
    int m_userId = -1;
    QString m_username;
    QString m_role;
    bool m_isAdmin = false;
};

#endif // ACHIEVEMENTWINDOW_H
