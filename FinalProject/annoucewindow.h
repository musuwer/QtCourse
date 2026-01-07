#ifndef ANNOUCEWINDOW_H
#define ANNOUCEWINDOW_H

#include <QDialog>

namespace Ui {
class AnnouceWindow;
}

/**
 * AnnouceWindow：发布公告弹窗
 * UI控件名（来自 annouce_window.ui）：
 * - annouce_title_lineEdit
 * - textEdit
 * - cancel_pushButton
 * - send_pushButton
 */
class AnnouceWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AnnouceWindow(QWidget *parent = nullptr);
    ~AnnouceWindow();

    QString getTitle() const;
    QString getContent() const;

private:
    void initUi();
    void connectSignals();

private slots:
    void onSendClicked();
    void onCancelClicked();

private:
    Ui::AnnouceWindow *ui;
};

#endif // ANNOUCEWINDOW_H
