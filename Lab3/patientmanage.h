#ifndef PATIENTMANAGE_H
#define PATIENTMANAGE_H

#include <QWidget>

namespace Ui {
class PatientManage;
}

class PatientManage : public QWidget
{
    Q_OBJECT

public:
    explicit PatientManage(QWidget *parent = nullptr);
    ~PatientManage();

private slots:
    void on_btnSearch_clicked();

    void on_btnAdd_clicked();

    void on_btnDelete_clicked();

    void on_btnUpdate_clicked();


signals:
    void goPatientEditView(int column);


private:
    Ui::PatientManage *ui;
};

#endif // PATIENTMANAGE_H
