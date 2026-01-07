#ifndef REPLYWINDOW_H
#define REPLYWINDOW_H

#include <QDialog>

namespace Ui {
class ReplyWindow;
}

/**
 * ReplyWindow：管理员回复消息弹窗
 * UI控件名（来自 reply_window.ui）：
 * - textEdit
 * - cancel_pushButton
 * - send_pushButton
 */
class ReplyWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ReplyWindow(QWidget *parent = nullptr);
    ~ReplyWindow();

    QString getReplyText() const;
    void setHint(const QString& hint);

private:
    void initUi();
    void connectSignals();

private slots:
    void onSendClicked();
    void onCancelClicked();

private:
    Ui::ReplyWindow *ui;
};

#endif // REPLYWINDOW_H
