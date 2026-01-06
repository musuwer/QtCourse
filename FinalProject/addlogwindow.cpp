#include "addlogwindow.h"
#include "ui_add_log_window.h"

#include <QMessageBox>

static int clampScore(int v)
{
    if (v < 0) return 0;
    if (v > 10) return 10;
    return v;
}

AddLogWindow::AddLogWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddLogWindow)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
}

AddLogWindow::~AddLogWindow()
{
    delete ui;
}

void AddLogWindow::initUi()
{
    setWindowTitle("添加事件记录");
    setModal(true);
}

void AddLogWindow::connectSignals()
{
    connect(ui->add_book_pushButton, &QPushButton::clicked, this, &AddLogWindow::onAddClicked);
}

void AddLogWindow::onAddClicked()
{
    if (getTitle().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "事件标题不能为空。");
        return;
    }
    accept();
}

QString AddLogWindow::getTitle() const
{
    return ui->book_name_lineEdit->text().trimmed();
}

QString AddLogWindow::getPerson() const
{
    return ui->author_lineEdit->text().trimmed();
}

QString AddLogWindow::getPlace() const
{
    return ui->publish_company_lineEdit->text().trimmed();
}

QString AddLogWindow::getDateStr() const
{
    return ui->publish_date_lineEdit->text().trimmed();
}

int AddLogWindow::getMoodScore() const
{
    bool ok=false;
    int v = ui->store_num_lineEdit->text().trimmed().toInt(&ok);
    if (!ok) v = 0;
    return clampScore(v);
}
