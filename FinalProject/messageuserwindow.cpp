#include "messageuserwindow.h"
#include "ui_message_info_user_window.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDate>
#include <QMap>
#include <QVBoxLayout>

#include "dbmanager.h"
// QtCharts：每月意见数量柱状图
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractAxis>


namespace {
static QString safeStr(const QVariant& v) { return v.isNull() ? "" : v.toString(); }
}

MessageUserWindow::MessageUserWindow(const QString& username, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MessageUserWindow)
    , m_username(username)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
    initChartsIfNeeded();
    refreshData();
}

MessageUserWindow::~MessageUserWindow()
{
    delete ui;
}

void MessageUserWindow::initUi()
{
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MessageUserWindow::connectSignals()
{
    connect(ui->send_pushButton, &QPushButton::clicked, this, &MessageUserWindow::onSendClicked);
    connect(ui->refresh_pushButton, &QPushButton::clicked, this, &MessageUserWindow::onRefreshClicked);
}

void MessageUserWindow::refreshData()
{
    ui->tableWidget->setRowCount(0);

    QSqlQuery q(DbManager::instance().database());
    q.prepare(R"SQL(
        SELECT sender, message, send_time, reply, reply_time
        FROM messages
        WHERE sender=? AND receiver='admin'
        ORDER BY id DESC
    )SQL");
    q.addBindValue(m_username);

    if (!q.exec()) {
        QMessageBox::warning(this, "查询失败", q.lastError().text());
        return;
    }

    int row=0;
    while (q.next()) {
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(safeStr(q.value(0))));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(safeStr(q.value(1))));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(safeStr(q.value(2))));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(safeStr(q.value(3))));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(safeStr(q.value(4))));
        row++;
    }

    ui->message_total_label->setText(QString("消息总数：%1").arg(row));

    updateMonthChart();
}

void MessageUserWindow::initChartsIfNeeded()
{
    if (m_monthChartView) return;
    if (!ui) return;

    QWidget* host = ui->advise_widget;
    if (!host) host = findChild<QWidget*>(QStringLiteral("advise_widget"));
    if (!host) return;

    if (!host->layout()) {
        auto* lay = new QVBoxLayout(host);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);
    }

    auto* chart = new QChart();
    chart->setTitle(QStringLiteral("每月意见数量"));
    chart->setAnimationOptions(QChart::NoAnimation);

    m_monthChartView = new QChartView(chart, host);
    m_monthChartView->setRenderHint(QPainter::Antialiasing, true);
    m_monthChartView->setRubberBand(QChartView::NoRubberBand);
    host->layout()->addWidget(m_monthChartView);
}

void MessageUserWindow::updateMonthChart()
{
    initChartsIfNeeded();
    if (!m_monthChartView) return;

    // 最近 12 个月（含本月）
    const QDate now = QDate::currentDate();
    QStringList categories;
    QMap<QString, int> counts;

    for (int i = 11; i >= 0; --i) {
        const QDate d = now.addMonths(-i);
        const QString ym = d.toString("yyyy-MM");
        categories << ym;
        counts[ym] = 0;
    }

    const QString fromYm = categories.first();
    const QString toYm   = categories.last();

    // sender = 当前用户，receiver = admin
    QSqlQuery q(DbManager::instance().database());
    q.prepare(R"SQL(
        SELECT substr(send_time, 1, 7) AS ym, COUNT(*) AS cnt
        FROM messages
        WHERE sender=? AND receiver='admin'
          AND substr(send_time, 1, 7) BETWEEN ? AND ?
        GROUP BY ym
        ORDER BY ym
    )SQL");
    q.addBindValue(m_username);
    q.addBindValue(fromYm);
    q.addBindValue(toYm);

    if (!q.exec()) {
        // 查询失败时不弹窗（避免打扰），仅保留空图
    } else {
        while (q.next()) {
            const QString ym = q.value(0).toString();
            const int cnt = q.value(1).toInt();
            if (counts.contains(ym)) counts[ym] = cnt;
        }
    }

    auto* set = new QBarSet(QStringLiteral("数量"));
    int maxV = 0;
    for (const auto& ym : categories) {
        const int v = counts.value(ym, 0);
        *set << v;
        maxV = qMax(maxV, v);
    }

    auto* series = new QBarSeries();
    series->append(set);
    series->setLabelsVisible(true);

    QChart* chart = m_monthChartView->chart();
    chart->removeAllSeries();
    for (auto* ax : chart->axes()) {
        chart->removeAxis(ax);
        ax->deleteLater();
    }

    chart->addSeries(series);
    chart->setTitle(QStringLiteral("每月意见数量"));
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis();
    axisY->setLabelFormat("%d");
    axisY->setMin(0);
    axisY->setMax(qMax(1, maxV));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
}


void MessageUserWindow::onSendClicked()
{
    const QString msg = ui->message_lineEdit->text().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入消息内容。");
        return;
    }

    QString err;
    if (!DbManager::instance().addMessage(m_username, "admin", msg, &err)) {
        QMessageBox::warning(this, "发送失败", err);
        return;
    }

    ui->message_lineEdit->clear();
    refreshData();
}

void MessageUserWindow::onRefreshClicked()
{
    refreshData();
}
