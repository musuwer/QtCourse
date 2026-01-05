#ifndef GOALRECORDWINDOW_H
#define GOALRECORDWINDOW_H

#include <QDialog>

namespace Ui {
class GoalRecordWindow;
}

/**
 * GoalRecordWindow：添加目标弹窗
 * UI控件名（来自 goal_record_window.ui）：
 * - lineEdit
 * - pushButton
 */
class GoalRecordWindow : public QDialog
{
    Q_OBJECT

public:
    explicit GoalRecordWindow(QWidget *parent = nullptr);
    ~GoalRecordWindow();

    QString getGoalText() const;

private:
    void initUi();
    void connectSignals();

private slots:
    void onOkClicked();

private:
    Ui::GoalRecordWindow *ui;
};

#endif // GOALRECORDWINDOW_H
