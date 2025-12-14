#ifndef MASTERVIEW_H
#define MASTERVIEW_H

#include <QWidget>
#include "loginview.h"
#include "doctorView.h"
#include "patienteditview.h"
#include "patientmanage.h"
#include "sectionview.h"
#include "welcomeView.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MasterView; }
QT_END_NAMESPACE

class MasterView : public QWidget
{
    Q_OBJECT

public:
    MasterView(QWidget *parent = nullptr);
    MasterView(Ui::MasterView *ui);
    ~MasterView();

public slots:
    void goLoginView();
    void goWelcomView();
    void goDoctorView();
    void goSectionView();
    void goPatientView();
    void goPatientEditView(int rowNum);
    void goPreviousView();

private slots:
    void on_btnBack_clicked();

    void on_btnLogout_clicked();

    void on_stackedWidget_currentChanged(int arg1);

private:

    void pushWidgetTOStackView(QWidget *widget);

    Ui::MasterView *ui;

    LoginView *loginView;
    WelcomeView *welcomView;
    DoctorView *doctorView;
    PatientManage *patientView;
    SectionView *sectionView;
    patientEditView *patientEditView1;

};
#endif // MASTERVIEW_H
