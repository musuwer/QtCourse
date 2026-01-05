#include "goalrecordwindow.h"
#include "ui_goal_record_window.h"

#include <QMessageBox>

GoalRecordWindow::GoalRecordWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GoalRecordWindow)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
}

GoalRecordWindow::~GoalRecordWindow()
{
    delete ui;
}

void GoalRecordWindow::initUi()
{
    setWindowTitle("添加目标");
    setModal(true);
}

void GoalRecordWindow::connectSignals()
{
    connect(ui->pushButton, &QPushButton::clicked, this, &GoalRecordWindow::onOkClicked);
}

void GoalRecordWindow::onOkClicked()
{
    if (getGoalText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "目标内容不能为空。");
        return;
    }
    accept();
}

QString GoalRecordWindow::getGoalText() const
{
    return ui->lineEdit->text().trimmed();
}
