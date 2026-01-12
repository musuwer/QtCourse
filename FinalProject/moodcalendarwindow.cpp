#include "moodcalendarwindow.h"
#include "ui_mood_calendar_window.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStringConverter>
#include <QDir>
#include <QVector>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QSignalBlocker>

// QtCharts
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractAxis>

#include "dbmanager.h"

namespace {
static QString safeStr(const QVariant& v) { return v.isNull() ? QString() : v.toString(); }

static QDate parseIsoDateOnly(const QString& s)
{
    // mood_date 约定 yyyy-MM-dd
    QDate d = QDate::fromString(s.trimmed(), Qt::ISODate);
    if (d.isValid()) return d;

    // 兼容：如果有人把 logs 的 yyyy-MM-dd HH:mm 放进来了
    const QString t = s.left(10);
    d = QDate::fromString(t, Qt::ISODate);
    return d;
}

static int clampScore(int v)
{
    if (v < 0) return 0;
    if (v > 10) return 10;
    return v;
}
}

MoodCalendarWindow::MoodCalendarWindow(int userId, const QString& username, const QString& role, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MoodCalendarWindow)
    , m_userId(userId)
    , m_username(username)
    , m_role(role)
{
    ui->setupUi(this);
    initUi();
    connectSignals();

    refreshForSelectedDate();
    refreshChartForMonth(ui->calendarWidget->selectedDate());
}

MoodCalendarWindow::~MoodCalendarWindow()
{
    delete ui;
}

void MoodCalendarWindow::initUi()
{
    // -------- table --------
    ui->annou_info_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->annou_info_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->annou_info_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->annou_info_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->annou_info_tableWidget->horizontalHeader()->setStretchLastSection(true);

    // 普通用户不显示 user_id 列
    if (m_role != "admin") {
        ui->annou_info_tableWidget->hideColumn(0);
    }

    // -------- chart --------
    if (ui->mood_line_widget) {
        auto* lay = new QVBoxLayout(ui->mood_line_widget);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        m_chart = new QChart();
        m_chart->setTitle(QStringLiteral("本月情绪评分折线"));

        m_chartView = new QChartView(m_chart, ui->mood_line_widget);
        m_chartView->setRenderHint(QPainter::Antialiasing);
        lay->addWidget(m_chartView);
    }
}

void MoodCalendarWindow::connectSignals()
{
    // ✅ 单击日历日期：直接编辑/新增该日心情
    connect(ui->calendarWidget, &QCalendarWidget::clicked, this, &MoodCalendarWindow::onCalendarClicked);

    // 选中日期变化：刷新右侧表格
    connect(ui->calendarWidget, &QCalendarWidget::selectionChanged, this, &MoodCalendarWindow::refreshForSelectedDate);

    // 翻月：更新折线图
    connect(ui->calendarWidget, &QCalendarWidget::currentPageChanged, this, &MoodCalendarWindow::onCalendarPageChanged);

    // ✅ 单击表格行：编辑该行对应日期/用户的心情
    connect(ui->annou_info_tableWidget, &QTableWidget::cellClicked, this, &MoodCalendarWindow::onTableCellClicked);

    // 导出按钮
    if (ui->export_csv_pushButton) {
        connect(ui->export_csv_pushButton, &QPushButton::clicked, this, &MoodCalendarWindow::onExportCsvClicked);
    }
}

QString MoodCalendarWindow::moodTextFromScore(int score) const
{
    if (score <= 2) return QStringLiteral("难过");
    if (score <= 4) return QStringLiteral("一般");
    if (score <= 6) return QStringLiteral("平静");
    if (score <= 8) return QStringLiteral("开心");
    return QStringLiteral("兴奋");
}

bool MoodCalendarWindow::getMoodRecord(int userId, const QDate& date,
                                      int* scoreOut, QString* moodTextOut, QString* errOut) const
{
    if (scoreOut) *scoreOut = 0;
    if (moodTextOut) *moodTextOut = QString();

    if (!DbManager::instance().isOpen()) {
        if (errOut) *errOut = QStringLiteral("数据库未打开");
        return false;
    }

    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT mood_score, mood_text FROM mood_records WHERE user_id=? AND mood_date=? LIMIT 1");
    q.addBindValue(userId);
    q.addBindValue(date.toString("yyyy-MM-dd"));

    if (!q.exec()) {
        if (errOut) *errOut = q.lastError().text();
        return false;
    }

    if (!q.next()) {
        // 不存在
        return true;
    }

    if (scoreOut) *scoreOut = q.value(0).toInt();
    if (moodTextOut) *moodTextOut = safeStr(q.value(1));
    return true;
}

bool MoodCalendarWindow::upsertMoodRecord(int userId, const QDate& date, int score, QString* errOut)
{
    if (!DbManager::instance().isOpen()) {
        if (errOut) *errOut = QStringLiteral("数据库未打开");
        return false;
    }

    score = clampScore(score);
    const QString moodText = moodTextFromScore(score);

    QSqlQuery q(DbManager::instance().database());
    q.prepare(R"SQL(
        INSERT INTO mood_records(user_id, mood_date, mood_score, mood_text, updated_at)
        VALUES(?,?,?,?,datetime('now'))
        ON CONFLICT(user_id, mood_date)
        DO UPDATE SET
            mood_score=excluded.mood_score,
            mood_text=excluded.mood_text,
            updated_at=datetime('now');
    )SQL");
    q.addBindValue(userId);
    q.addBindValue(date.toString("yyyy-MM-dd"));
    q.addBindValue(score);
    q.addBindValue(moodText);

    if (!q.exec()) {
        if (errOut) *errOut = q.lastError().text();
        return false;
    }
    return true;
}

int MoodCalendarWindow::pickUserIdForAdmin(QString* errOut) const
{
    // 管理员在日历直接点某天时，需要先选一个用户
    QSqlQuery q(DbManager::instance().database());
    if (!q.exec("SELECT id, username FROM users ORDER BY id")) {
        if (errOut) *errOut = q.lastError().text();
        return -1;
    }

    QStringList items;
    QList<int> ids;
    while (q.next()) {
        const int id = q.value(0).toInt();
        const QString username = safeStr(q.value(1));
        items << QString("%1 (ID=%2)").arg(username).arg(id);
        ids << id;
    }

    if (items.isEmpty()) {
        if (errOut) *errOut = QStringLiteral("users 表为空");
        return -1;
    }

    bool ok = false;
    const QString chosen = QInputDialog::getItem(
        const_cast<MoodCalendarWindow*>(this),
        QStringLiteral("选择用户"),
        QStringLiteral("请选择要编辑哪位用户的心情："),
        items,
        0,
        false,
        &ok
    );
    if (!ok) return -1;

    const int idx = items.indexOf(chosen);
    if (idx < 0 || idx >= ids.size()) return -1;
    return ids[idx];
}

void MoodCalendarWindow::refreshForSelectedDate()
{
    if (m_updatingTable) return;

    m_updatingTable = true;
    QSignalBlocker blocker(ui->annou_info_tableWidget);

    ui->annou_info_tableWidget->setRowCount(0);

    const QDate d = ui->calendarWidget->selectedDate();
    const QString dateStr = d.toString("yyyy-MM-dd");

    QSqlQuery q(DbManager::instance().database());
    if (m_role == "admin") {
        q.prepare(R"SQL(
            SELECT user_id, mood_date, mood_text, mood_score
            FROM mood_records
            WHERE mood_date=?
            ORDER BY user_id ASC
        )SQL");
        q.addBindValue(dateStr);
    } else {
        q.prepare(R"SQL(
            SELECT user_id, mood_date, mood_text, mood_score
            FROM mood_records
            WHERE user_id=? AND mood_date=?
            LIMIT 1
        )SQL");
        q.addBindValue(m_userId);
        q.addBindValue(dateStr);
    }

    if (!q.exec()) {
        QMessageBox::warning(this, QStringLiteral("查询失败"), q.lastError().text());
        m_updatingTable = false;
        return;
    }

    int row = 0;
    while (q.next()) {
        const int uid = q.value(0).toInt();
        const QString dstr = safeStr(q.value(1));
        const QString moodText = safeStr(q.value(2));
        const int score = q.value(3).toInt();

        ui->annou_info_tableWidget->insertRow(row);
        ui->annou_info_tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(uid)));
        ui->annou_info_tableWidget->setItem(row, 1, new QTableWidgetItem(dstr));
        ui->annou_info_tableWidget->setItem(row, 2, new QTableWidgetItem(moodText.isEmpty() ? moodTextFromScore(score) : moodText));
        ui->annou_info_tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(score)));
        row++;
    }

    m_updatingTable = false;

    // 折线图跟随刷新（月维度）
    refreshChartForMonth(d);
}

void MoodCalendarWindow::clearChart()
{
    if (!m_chart) return;

    m_chart->removeAllSeries();
    const auto axes = m_chart->axes();
    for (auto* ax : axes) {
        m_chart->removeAxis(ax);
        delete ax;
    }
}

void MoodCalendarWindow::refreshChartForMonth(const QDate& anyDateInMonth)
{
    if (!m_chart) return;

    const QDate monthStart(anyDateInMonth.year(), anyDateInMonth.month(), 1);
    const QDate monthEnd = monthStart.addMonths(1).addDays(-1);

    clearChart();

    auto* axisX = new QDateTimeAxis();
    axisX->setFormat("MM-dd");
    axisX->setTitleText(QStringLiteral("日期"));
    axisX->setRange(QDateTime(monthStart, QTime(0,0,0)), QDateTime(monthEnd, QTime(23,59,59)));

    auto* axisY = new QValueAxis();
    axisY->setTitleText(QStringLiteral("情绪评分"));
    axisY->setRange(0, 10);
    axisY->setTickCount(6);
    axisY->setLabelFormat("%d");

    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_chart->addAxis(axisY, Qt::AlignLeft);

    const QString startStr = monthStart.toString("yyyy-MM-dd");
    const QString endStr = monthEnd.toString("yyyy-MM-dd");

    if (m_role == "admin") {
        m_chart->setTitle(QStringLiteral("所有用户 · 本月情绪折线"));
        m_chart->legend()->setVisible(true);
        m_chart->legend()->setAlignment(Qt::AlignRight);

        QSqlQuery q(DbManager::instance().database());
        q.prepare(R"SQL(
            SELECT u.username, m.user_id, m.mood_date, m.mood_score
            FROM mood_records m
            JOIN users u ON u.id = m.user_id
            WHERE m.mood_date BETWEEN ? AND ?
            ORDER BY m.user_id ASC, m.mood_date ASC
        )SQL");
        q.addBindValue(startStr);
        q.addBindValue(endStr);

        if (!q.exec()) {
            // 不弹窗也行，避免影响使用
            return;
        }

        QMap<int, QLineSeries*> seriesByUser;
        QMap<int, QString> nameByUser;

        while (q.next()) {
            const QString username = safeStr(q.value(0));
            const int uid = q.value(1).toInt();
            const QDate d = parseIsoDateOnly(safeStr(q.value(2)));
            const int score = q.value(3).toInt();
            if (!d.isValid()) continue;

            if (!seriesByUser.contains(uid)) {
                auto* s = new QLineSeries();
                s->setName(username.isEmpty() ? QString("UID=%1").arg(uid) : username);
                seriesByUser.insert(uid, s);
                nameByUser.insert(uid, s->name());
            }

            const QDateTime dt(d, QTime(0,0,0));
            seriesByUser[uid]->append(dt.toMSecsSinceEpoch(), clampScore(score));
        }

        for (auto it = seriesByUser.begin(); it != seriesByUser.end(); ++it) {
            m_chart->addSeries(it.value());
            it.value()->attachAxis(axisX);
            it.value()->attachAxis(axisY);
        }

    } else {
        m_chart->setTitle(QStringLiteral("我的本月情绪折线"));
        m_chart->legend()->setVisible(false);

        auto* series = new QLineSeries();
        series->setName(m_username);

        QSqlQuery q(DbManager::instance().database());
        q.prepare(R"SQL(
            SELECT mood_date, mood_score
            FROM mood_records
            WHERE user_id=? AND mood_date BETWEEN ? AND ?
            ORDER BY mood_date ASC
        )SQL");
        q.addBindValue(m_userId);
        q.addBindValue(startStr);
        q.addBindValue(endStr);

        if (!q.exec()) {
            return;
        }

        while (q.next()) {
            const QDate d = parseIsoDateOnly(safeStr(q.value(0)));
            const int score = q.value(1).toInt();
            if (!d.isValid()) continue;
            series->append(QDateTime(d, QTime(0,0,0)).toMSecsSinceEpoch(), clampScore(score));
        }

        m_chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }
}

void MoodCalendarWindow::onCalendarClicked(const QDate& date)
{
    // 点击日历某天：编辑那天的心情
    int targetUserId = m_userId;
    if (m_role == "admin") {
        QString err;
        targetUserId = pickUserIdForAdmin(&err);
        if (targetUserId < 0) return;
    }

    int currentScore = 5;
    QString currentMood;
    QString err;
    getMoodRecord(targetUserId, date, &currentScore, &currentMood, &err);

    bool ok = false;
    const int newScore = QInputDialog::getInt(
        this,
        QStringLiteral("编辑心情评分"),
        QStringLiteral("请选择 %1 的 %2 情绪评分（0~10）：")
            .arg(m_role == "admin" ? QString("UID=%1").arg(targetUserId) : m_username)
            .arg(date.toString("yyyy-MM-dd")),
        clampScore(currentScore),
        0,
        10,
        1,
        &ok
    );
    if (!ok) return;

    if (!upsertMoodRecord(targetUserId, date, newScore, &err)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), err);
        return;
    }

    refreshForSelectedDate();
    refreshChartForMonth(date);
}

void MoodCalendarWindow::onCalendarPageChanged(int year, int month)
{
    // 翻月时：折线图跟随切换到该月
    const QDate d(year, month, 1);
    refreshChartForMonth(d);
}

void MoodCalendarWindow::onTableCellClicked(int row, int /*column*/)
{
    if (m_updatingTable) return;
    if (!ui->annou_info_tableWidget) return;
    if (row < 0 || row >= ui->annou_info_tableWidget->rowCount()) return;

    auto* uidItem = ui->annou_info_tableWidget->item(row, 0);
    auto* dateItem = ui->annou_info_tableWidget->item(row, 1);
    if (!uidItem || !dateItem) return;

    const int uid = uidItem->text().trimmed().toInt();
    const QDate date = parseIsoDateOnly(dateItem->text());
    if (uid <= 0 || !date.isValid()) return;

    // 先同步日历选中日期（不触发 clicked）
    ui->calendarWidget->setSelectedDate(date);

    // 再进入编辑（管理员/用户都按行的 uid 来）
    int currentScore = 5;
    QString currentMood;
    QString err;
    getMoodRecord(uid, date, &currentScore, &currentMood, &err);

    bool ok = false;
    const int newScore = QInputDialog::getInt(
        this,
        QStringLiteral("编辑心情评分"),
        QStringLiteral("请选择 UID=%1 的 %2 情绪评分（0~10）：")
            .arg(uid)
            .arg(date.toString("yyyy-MM-dd")),
        clampScore(currentScore),
        0,
        10,
        1,
        &ok
    );
    if (!ok) return;

    if (!upsertMoodRecord(uid, date, newScore, &err)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), err);
        return;
    }

    refreshForSelectedDate();
    refreshChartForMonth(date);
}

void MoodCalendarWindow::onExportCsvClicked()
{
    if (!ui->annou_info_tableWidget) return;
    if (ui->annou_info_tableWidget->rowCount() <= 0) {
        QMessageBox::information(this, QStringLiteral("导出"), QStringLiteral("没有可导出的数据。"));
        return;
    }

    const QDate d = ui->calendarWidget->selectedDate();
    const QString defaultName = QString("mood_%1_%2.csv")
        .arg(d.toString("yyyyMMdd"))
        .arg(QDateTime::currentDateTime().toString("HHmmss"));
    const QString defaultPath = QDir::home().filePath(defaultName);
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("导出心情记录为 CSV"), defaultPath, QStringLiteral("CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(".csv", Qt::CaseInsensitive)) filePath += ".csv";

    // 导出可见列
    QVector<int> cols;
    cols.reserve(ui->annou_info_tableWidget->columnCount());
    for (int c = 0; c < ui->annou_info_tableWidget->columnCount(); ++c) {
        if (!ui->annou_info_tableWidget->isColumnHidden(c)) cols.push_back(c);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("无法写入文件：") + file.errorString());
        return;
    }

    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    out << QChar(0xFEFF);

    auto csvEscape = [](QString s) -> QString {
        s.replace("\"", "\"\"");
        if (s.contains(',') || s.contains('"') || s.contains('\n') || s.contains('\r')) {
            s = "\"" + s + "\"";
        }
        return s;
    };

    // 表头
    {
        QStringList header;
        header.reserve(cols.size());
        for (int c : cols) {
            auto* hi = ui->annou_info_tableWidget->horizontalHeaderItem(c);
            header << csvEscape(hi ? hi->text() : QString("col_%1").arg(c));
        }
        out << header.join(",") << "\n";
    }

    // 数据
    int exported = 0;
    for (int r = 0; r < ui->annou_info_tableWidget->rowCount(); ++r) {
        if (ui->annou_info_tableWidget->isRowHidden(r)) continue;
        QStringList row;
        row.reserve(cols.size());
        for (int c : cols) {
            const QTableWidgetItem* it = ui->annou_info_tableWidget->item(r, c);
            row << csvEscape(it ? it->text() : QString());
        }
        out << row.join(",") << "\n";
        exported++;
    }

    file.close();
    QMessageBox::information(this,
                             QStringLiteral("导出完成"),
                             QStringLiteral("已导出 %1 条心情记录到：\n%2")
                                 .arg(exported)
                                 .arg(QDir::toNativeSeparators(filePath)));
}
