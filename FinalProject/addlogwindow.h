#ifndef ADDLOGWINDOW_H
#define ADDLOGWINDOW_H

#include <QDialog>

namespace Ui {
class AddLogWindow;
}

/**
 * AddLogWindow：添加“事件记录”的弹窗
 * 注意：控件名必须与 add_log_window.ui 中一致：
 * - book_name_lineEdit
 * - author_lineEdit
 * - publish_company_lineEdit
 * - publish_date_lineEdit
 * - store_num_lineEdit
 * - add_book_pushButton
 */
class AddLogWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AddLogWindow(QWidget *parent = nullptr);
    ~AddLogWindow();

    QString getTitle() const;
    QString getPerson() const;
    QString getPlace() const;
    QString getDateStr() const;
    int getMoodScore() const;

private:
    void initUi();
    void connectSignals();

private slots:
    void onAddClicked();

private:
    Ui::AddLogWindow *ui;
};

#endif // ADDLOGWINDOW_H
