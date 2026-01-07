#ifndef MESSAGEUSERWINDOW_H
#define MESSAGEUSERWINDOW_H

#include <QWidget>
#include <QString>

namespace Ui {
class MessageUserWindow;
}

class MessageUserWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MessageUserWindow(const QString& username, QWidget *parent = nullptr);
    ~MessageUserWindow();

public slots:
    void refreshData();

private:
    void initUi();
    void connectSignals();

private slots:
    void onSendClicked();
    void onRefreshClicked();

private:
    Ui::MessageUserWindow *ui;
    QString m_username;
};

#endif // MESSAGEUSERWINDOW_H
