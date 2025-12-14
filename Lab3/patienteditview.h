#ifndef PATIENTEDITVIEW_H
#define PATIENTEDITVIEW_H

#include <QWidget>
#include <QDataWidgetMapper>

namespace Ui {
class patientEditView;
}

class patientEditView : public QWidget
{
    Q_OBJECT

public:
    explicit patientEditView(QWidget *parent = nullptr,int rowNum=0);
    ~patientEditView();

private slots:
    void on_btnSave_clicked();

    void on_btnCancel_clicked();

private:
    Ui::patientEditView *ui;
    //数据映射
    QDataWidgetMapper *dataMapper;
signals:
    void goPreviousView();
};

#endif // PATIENTEDITVIEW_H
