#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QWidget>
#include <QPoint>

#ifdef Q_OS_WIN
#include <QByteArray>
#endif

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
    bool eventFilter(QObject *obj, QEvent *event) override;

#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

signals:
    void backToLoginRequested();

private slots:
    void handleRegister();
    void handleReturn();

private:
    Ui::RegisterWindow *ui;

    void setupFrameless();
    void bindWindowButtons();

    bool m_dragging = false;
    QPoint m_dragPos;
};

#endif // REGISTERWINDOW_H
