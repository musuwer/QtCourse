#include "moodcalendarwindow.h"
#include "ui_mood_calendar_window.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDate>

#include "dbmanager.h"

namespace {
static QString safeStr(const QVariant& v) { return v.isNull() ? "" : v.toString(); }
}

MoodCalendarWindow::MoodCalendarWindow(int userId, const QString& username, const QString& role, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MoodCalendarWindow)
    , m_userId(userId)
    , m_username(username)
    , m_role(role)
    , m_isAdmin(role == "admin")
{
    ui->setupUi(this);
    initUi();
    connectSignals();
    refreshForSelectedDate();
}

MoodCalendarWindow::~MoodCalendarWindow()
{
    delete ui;
}

void MoodCalendarWindow::initUi()
{
    ui->annou_info_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->annou_info_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->annou_info_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MoodCalendarWindow::connectSignals()
{
    connect(ui->calendarWidget, &QCalendarWidget::selectionChanged, this, &MoodCalendarWindow::onCalendarChanged);
}

void MoodCalendarWindow::onCalendarChanged()
{
    refreshForSelectedDate();
}

QString MoodCalendarWindow::moodTextFromScore(int score) const
{
    if (score <= 2) return "难过";
    if (score <= 4) return "一般";
    if (score <= 6) return "平静";
    if (score <= 8) return "开心";
    return "兴奋";
}

void MoodCalendarWindow::refreshForSelectedDate()
{
    ui->annou_info_tableWidget->setRowCount(0);

    const QDate d = ui->calendarWidget->selectedDate();
    const QString dateStr = d.toString("yyyy-MM-dd");

    // logs: id,user_id,title,person,place,log_date,mood_score,created_at
    QSqlQuery q(DbManager::instance().database());
    if (m_isAdmin) {
        q.prepare(R"SQL(
            SELECT user_id, log_date, title, mood_score
            FROM logs
            WHERE (log_date=? OR log_date LIKE ?)
            ORDER BY id DESC
        )SQL");
        q.addBindValue(dateStr);
        q.addBindValue(dateStr + "%");
    } else {
        q.prepare(R"SQL(
            SELECT user_id, log_date, title, mood_score
            FROM logs
            WHERE user_id=? AND (log_date=? OR log_date LIKE ?)
            ORDER BY id DESC
        )SQL");
        q.addBindValue(m_userId);
        q.addBindValue(dateStr);
        q.addBindValue(dateStr + "%");
    }

    if (!q.exec()) {
        QMessageBox::warning(this, "查询失败", q.lastError().text());
        return;
    }

    int row=0;
    while (q.next()) {
        ui->annou_info_tableWidget->insertRow(row);

        const int uid = q.value(0).toInt();
        const QString dstr = safeStr(q.value(1));
        const QString title = safeStr(q.value(2));
        const int score = q.value(3).toInt();

        ui->annou_info_tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(uid)));
        ui->annou_info_tableWidget->setItem(row, 1, new QTableWidgetItem(dstr));
        ui->annou_info_tableWidget->setItem(row, 2, new QTableWidgetItem(QString("%1 (%2)").arg(moodTextFromScore(score)).arg(title)));
        ui->annou_info_tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(score)));

        row++;
    }
}
