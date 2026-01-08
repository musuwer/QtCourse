#ifndef ACHIEVEMENTWINDOW_H
#define ACHIEVEMENTWINDOW_H

#include <QWidget>
#include <QString>

class QPushButton;
class QChartView; // QtCharts 的 QChartView（Qt6 下通常在全局命名空间）

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

    // 图表：左侧柱状图（按成就类型统计），右侧扇形图（按成就级别统计）
    void initChartsIfNeeded();
    void updateChartsFromTable();

private slots:
    void onAddClicked();
    void onRefreshClicked();
    void onSearchClicked();
    void onExportCsvClicked();
    void onTableContextMenu(const QPoint& pos);

private:
    Ui::AchievementWindow *ui;
    int m_userId = -1;
    QString m_username;
    QString m_role;

    QPushButton* m_exportCsvBtn = nullptr;

    QChartView* m_typeChartView = nullptr;
    QChartView* m_levelPieView = nullptr;
};

#endif // ACHIEVEMENTWINDOW_H
