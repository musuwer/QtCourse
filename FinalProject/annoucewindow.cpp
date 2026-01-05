#include "annoucewindow.h"
#include "ui_annouce_window.h"

#include <QMessageBox>

AnnouceWindow::AnnouceWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AnnouceWindow)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
}

AnnouceWindow::~AnnouceWindow()
{
    delete ui;
}

void AnnouceWindow::initUi()
{
    setWindowTitle("发布公告");
    setModal(true);
}

void AnnouceWindow::connectSignals()
{
    connect(ui->send_pushButton, &QPushButton::clicked, this, &AnnouceWindow::onSendClicked);
    connect(ui->cancel_pushButton, &QPushButton::clicked, this, &AnnouceWindow::onCancelClicked);
}

void AnnouceWindow::onSendClicked()
{
    if (getTitle().trimmed().isEmpty() || getContent().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "公告标题和内容不能为空。");
        return;
    }
    accept();
}

void AnnouceWindow::onCancelClicked()
{
    reject();
}

QString AnnouceWindow::getTitle() const
{
    return ui->annouce_title_lineEdit->text().trimmed();
}

QString AnnouceWindow::getContent() const
{
    return ui->textEdit->toPlainText().trimmed();
}
