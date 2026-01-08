#ifndef LOGRECORDWINDOW_H
#define LOGRECORDWINDOW_H

#include <QWidget>
#include <QString>

class QSqlTableModel;
class QSortFilterProxyModel;
class QChartView;

namespace Ui {
class LogRecordWindow;
}

class LogRecordWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LogRecordWindow(int userId, const QString& username, const QString& role, QWidget *parent = nullptr);
    ~LogRecordWindow();

public slots:
    void refreshData();

private:
    void initUi();
    void connectSignals();
    void updateCountLabel();

    // Charts (QtCharts)
    void initChartsIfNeeded();
    void updateChartsFromTable();

private slots:
    void onAddLogClicked();
    void onRefreshClicked();
    void onSearchClicked();
    void onExportCsvClicked();

    void onTableContextMenuRequested(const QPoint& pos);

private:
    Ui::LogRecordWindow *ui;
    int m_userId = -1;
    QString m_username;
    QString m_role;

    QSqlTableModel* m_model = nullptr;
    QSortFilterProxyModel* m_proxy = nullptr;

    QChartView* m_pieChartView = nullptr;
    QChartView* m_lineChartView = nullptr;
};

#endif // LOGRECORDWINDOW_H
