#ifndef ANNOUCEWINDOW_H
#define ANNOUCEWINDOW_H

#include <QDialog>

namespace Ui {
class AnnouceWindow;
}

class AnnouceWindow : public QDialog
{
    Q_OBJECT

public:
    // 优化：增加 author 参数，默认值为 "Admin"
    explicit AnnouceWindow(const QString& author = "Admin", QWidget *parent = nullptr);
    ~AnnouceWindow();

    QString getTitle() const;
    QString getContent() const;

private slots:
    void onSendClicked();
    void onCancelClicked();

private:
    void initUi();
    void connectSignals();

    Ui::AnnouceWindow *ui;
    QString m_author; // 储存发布者名字
};

#endif // ANNOUCEWINDOW_H
