#ifndef MOODCALENDARWINDOW_H
#define MOODCALENDARWINDOW_H

#include <QWidget>
#include <QString>

class QPushButton;

namespace Ui {
class MoodCalendarWindow;
}

class MoodCalendarWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MoodCalendarWindow(int userId, const QString& username, const QString& role, QWidget *parent = nullptr);
    ~MoodCalendarWindow();

public slots:
    void refreshForSelectedDate();

private:
    void initUi();
    void connectSignals();
    QString moodTextFromScore(int score) const;

private slots:
    void onCalendarChanged();
    void onExportCsvClicked();

private:
    Ui::MoodCalendarWindow *ui;
    int m_userId = -1;
    QString m_username;
    QString m_role;

    QPushButton* m_exportCsvBtn = nullptr;
};

#endif // MOODCALENDARWINDOW_H
