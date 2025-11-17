#ifndef ISSAVEDIALOG_H
#define ISSAVEDIALOG_H

#include <QDialog>

namespace Ui {
class isSaveDialog;
}

class isSaveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit isSaveDialog(QWidget *parent = nullptr);
    ~isSaveDialog();

private slots:
    void on_saveButton_clicked();

private:
    Ui::isSaveDialog *ui;
};

#endif // ISSAVEDIALOG_H
