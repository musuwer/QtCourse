#ifndef MESSAGEUSERWINDOW_H
#define MESSAGEUSERWINDOW_H

#include <QWidget>
#include <QString>

class QChartView; // QtCharts

namespace Ui {
class MessageUserWindow;
}

class MessageUserWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MessageUserWindow(const QString& username, QWidget *parent = nullptr);
    ~MessageUserWindow();

public slots:
    void refreshData();

private:
    void initUi();
    void connectSignals();

    // 图表：每月意见数量
    void initChartsIfNeeded();
    void updateMonthChart();

private slots:
    void onSendClicked();
    void onRefreshClicked();

private:
    Ui::MessageUserWindow *ui;
    QString m_username;

    QChartView* m_monthChartView = nullptr;
};

#endif // MESSAGEUSERWINDOW_H
