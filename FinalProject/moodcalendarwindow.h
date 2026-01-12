#ifndef MOODCALENDARWINDOW_H
#define MOODCALENDARWINDOW_H

#include <QWidget>
#include <QString>
#include <QDate>

QT_BEGIN_NAMESPACE
namespace Ui { class MoodCalendarWindow; }
class QPushButton;
// Qt6: Qt Charts no longer uses the QtCharts namespace.
// Forward-declare chart types in Qt's namespace for compatibility.
class QChart;
class QChartView;
QT_END_NAMESPACE

class MoodCalendarWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MoodCalendarWindow(int userId,
                                const QString& username,
                                const QString& role,
                                QWidget *parent = nullptr);
    ~MoodCalendarWindow();

public slots:
    void refreshForSelectedDate();

private:
    void initUi();
    void connectSignals();

    QString moodTextFromScore(int score) const;

    bool getMoodRecord(int userId, const QDate& date,
                       int* scoreOut, QString* moodTextOut,
                       QString* errOut = nullptr) const;

    bool upsertMoodRecord(int userId, const QDate& date,
                          int score,
                          QString* errOut = nullptr);

    int pickUserIdForAdmin(QString* errOut = nullptr) const;

    void refreshChartForMonth(const QDate& anyDateInMonth);
    void clearChart();

private slots:
    void onCalendarClicked(const QDate& date);
    void onCalendarPageChanged(int year, int month);
    void onTableCellClicked(int row, int column);
    void onExportCsvClicked();

private:
    Ui::MoodCalendarWindow *ui = nullptr;

    int m_userId = -1;
    QString m_username;
    QString m_role;

    QPushButton* m_exportCsvBtn = nullptr;

    QChartView* m_chartView = nullptr;
    QChart* m_chart = nullptr;

    bool m_updatingTable = false;
};

#endif // MOODCALENDARWINDOW_H
