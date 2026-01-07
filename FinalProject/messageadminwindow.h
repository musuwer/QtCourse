#ifndef MESSAGEADMINWINDOW_H
#define MESSAGEADMINWINDOW_H

#include <QWidget>
#include <QString>

namespace Ui {
class MessageAdminWindow;
}

class MessageAdminWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MessageAdminWindow(const QString& adminName, QWidget *parent = nullptr);
    ~MessageAdminWindow();

public slots:
    void refreshData(bool onlyNoReply = false);

private:
    void initUi();
    void connectSignals();
    void loadData(const QString& senderFilter, bool onlyNoReply);

private slots:
    void onRefreshClicked();
    void onSearchClicked();
    void onNoReplyClicked();
    void onCellDoubleClicked(int row, int column);

private:
    Ui::MessageAdminWindow *ui;
    QString m_adminName;
};

#endif // MESSAGEADMINWINDOW_H
