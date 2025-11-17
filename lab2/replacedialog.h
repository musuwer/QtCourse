#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>

namespace Ui {
class replaceDialog;
}

class replaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit replaceDialog(QWidget *parent = nullptr,QPlainTextEdit * textEdit = nullptr);
    ~replaceDialog();

private slots:
    void on_findNextButton_clicked();

    void on_replaceButton_clicked();

    void on_replaceAllButton_clicked();

    void on_cancelButton_clicked();

private:
    Ui::replaceDialog *ui;

    QPlainTextEdit *pTextEdit;

    int count = 0;
};

#endif // REPLACEDIALOG_H
