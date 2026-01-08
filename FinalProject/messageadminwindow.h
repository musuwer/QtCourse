#ifndef MESSAGEADMINWINDOW_H
#define MESSAGEADMINWINDOW_H

#include <QWidget>
#include <QString>

class QChartView; // QtCharts

namespace Ui {
class MessageAdminWindow;
}

class MessageAdminWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MessageAdminWindow(const QString& adminName, QWidget *parent = nullptr);
    ~MessageAdminWindow();

public slots:
    void refreshData(bool onlyNoReply = false);

private:
    void initUi();
    void connectSignals();

    // 图表：每月意见数量
    void initChartsIfNeeded();
    void updateMonthChart(const QString& senderFilter, bool onlyNoReply);
    void loadData(const QString& senderFilter, bool onlyNoReply);

private slots:
    void onRefreshClicked();
    void onSearchClicked();
    void onNoReplyClicked();
    void onCellDoubleClicked(int row, int column);

private:
    Ui::MessageAdminWindow *ui;
    QString m_adminName;

    QChartView* m_monthChartView = nullptr;
};

#endif // MESSAGEADMINWINDOW_H
