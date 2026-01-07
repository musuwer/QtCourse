#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QWidget>

namespace Ui {
class RegisterWindow;
}

class RegisterWindow : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    ~RegisterWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void backToLoginRequested();

private slots:
    void handleRegister();
    void handleReturn();

private:
    void initFrameless();

    Ui::RegisterWindow *ui;

    bool m_dragging = false;
    QPoint m_dragOffset;
};

#endif // REGISTERWINDOW_H
