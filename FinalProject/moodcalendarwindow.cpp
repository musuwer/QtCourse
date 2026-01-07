#include "moodcalendarwindow.h"
#include "ui_mood_calendar_window.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDate>
#include <QHeaderView>

#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStringConverter>
#include <QDir>
#include <QVector>

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
    ui->annou_info_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->annou_info_tableWidget->horizontalHeader()->setStretchLastSection(true);
    if (m_role != "admin") {
        ui->annou_info_tableWidget->hideColumn(0); // 普通用户不显示 user_id
    }
}

void MoodCalendarWindow::connectSignals()
{
    connect(ui->calendarWidget, &QCalendarWidget::selectionChanged, this, &MoodCalendarWindow::onCalendarChanged);
    // ✅ 导出按钮由 .ui 提供
    if (ui->export_csv_pushButton) {
        connect(ui->export_csv_pushButton, &QPushButton::clicked, this, &MoodCalendarWindow::onExportCsvClicked);
    }
}

void MoodCalendarWindow::onCalendarChanged()
{
    refreshForSelectedDate();
}

void MoodCalendarWindow::onExportCsvClicked()
{
    if (!ui->annou_info_tableWidget) return;
    if (ui->annou_info_tableWidget->rowCount() <= 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    const QDate d = ui->calendarWidget->selectedDate();
    const QString defaultName = QString("mood_%1_%2.csv")
        .arg(d.toString("yyyyMMdd"))
        .arg(QDateTime::currentDateTime().toString("HHmmss"));
    const QString defaultPath = QDir::home().filePath(defaultName);
    QString filePath = QFileDialog::getSaveFileName(this, "导出心情记录为 CSV", defaultPath, "CSV 文件 (*.csv)");
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
        QMessageBox::warning(this, "导出失败", "无法写入文件：" + file.errorString());
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

    // 数据（跳过隐藏行）
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
    QMessageBox::information(this, "导出完成",
                             QString("已导出 %1 条心情记录到：\n%2").arg(exported).arg(QDir::toNativeSeparators(filePath)));
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
    if (m_role == "admin") {
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
