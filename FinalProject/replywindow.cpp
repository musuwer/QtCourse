#include "replywindow.h"
#include "ui_reply_window.h"

#include <QMessageBox>

ReplyWindow::ReplyWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReplyWindow)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
}

ReplyWindow::~ReplyWindow()
{
    delete ui;
}

void ReplyWindow::initUi()
{
    setWindowTitle("回复消息");
    setModal(true);
}

void ReplyWindow::connectSignals()
{
    connect(ui->send_pushButton, &QPushButton::clicked, this, &ReplyWindow::onSendClicked);
    connect(ui->cancel_pushButton, &QPushButton::clicked, this, &ReplyWindow::onCancelClicked);
}

void ReplyWindow::onSendClicked()
{
    if (getReplyText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "回复内容不能为空。");
        return;
    }
    accept();
}

void ReplyWindow::onCancelClicked()
{
    reject();
}

QString ReplyWindow::getReplyText() const
{
    return ui->textEdit->toPlainText().trimmed();
}

void ReplyWindow::setHint(const QString& hint)
{
    ui->textEdit->setPlaceholderText(hint);
}
