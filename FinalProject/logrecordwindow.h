#ifndef LOGRECORDWINDOW_H
#define LOGRECORDWINDOW_H

#include <QWidget>
#include <QSqlTableModel>

namespace Ui {
class LogRecordWindow;
}

class LogRecordWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LogRecordWindow(int userId, QWidget *parent = nullptr);
    ~LogRecordWindow();

    void refreshData(); // 刷新数据

private slots:
    void on_btnAddLog_clicked();
    void on_btnDeleteLog_clicked();

private:
    Ui::LogRecordWindow *ui;
    QSqlTableModel *m_model; // 核心：使用 SqlModel
    int m_currentUserId;
};

#endif // LOGRECORDWINDOW_H
