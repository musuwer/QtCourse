#include "messageadminwindow.h"
#include "ui_message_info_window.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDateTime>
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

#include "replywindow.h"

namespace {
static QString safeStr(const QVariant& v) { return v.isNull() ? "" : v.toString(); }
}

MessageAdminWindow::MessageAdminWindow(const QString& adminName, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MessageAdminWindow)
    , m_adminName(adminName)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
    initChartsIfNeeded();
    refreshData(false);
}

MessageAdminWindow::~MessageAdminWindow()
{
    delete ui;
}

void MessageAdminWindow::initUi()
{
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MessageAdminWindow::connectSignals()
{
    connect(ui->refresh_pushButton, &QPushButton::clicked, this, &MessageAdminWindow::onRefreshClicked);
    connect(ui->search_sender_pushButton, &QPushButton::clicked, this, &MessageAdminWindow::onSearchClicked);
    connect(ui->noreply_pushButton, &QPushButton::clicked, this, &MessageAdminWindow::onNoReplyClicked);
    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &MessageAdminWindow::onCellDoubleClicked);
}

void MessageAdminWindow::refreshData(bool onlyNoReply)
{
    const QString senderFilter = ui->sender_search_lineEdit->text().trimmed();
    loadData(senderFilter, onlyNoReply);
}

void MessageAdminWindow::loadData(const QString& senderFilter, bool onlyNoReply)
{
    ui->tableWidget->setRowCount(0);

    QString sql =
        "SELECT id,sender,receiver,message,replied,reply,send_time,reply_time "
        "FROM messages WHERE receiver='admin' ";

    if (!senderFilter.isEmpty()) {
        sql += "AND sender LIKE ? ";
    }
    if (onlyNoReply) {
        sql += "AND replied=0 ";
    }
    sql += "ORDER BY id DESC";

    QSqlQuery q(DbManager::instance().database());
    q.prepare(sql);
    if (!senderFilter.isEmpty()) {
        q.addBindValue("%" + senderFilter + "%");
    }

    if (!q.exec()) {
        QMessageBox::warning(this, "查询失败", q.lastError().text());
        return;
    }

    int row=0;
    while (q.next()) {
        ui->tableWidget->insertRow(row);

        const int msgId = q.value(0).toInt();
        const QString sender = safeStr(q.value(1));
        const QString receiver = safeStr(q.value(2));
        const QString message = safeStr(q.value(3));
        const int replied = q.value(4).toInt();
        const QString reply = safeStr(q.value(5));
        const QString sendTime = safeStr(q.value(6));
        const QString replyTime = safeStr(q.value(7));

        // 0 发送者：保存 msgId
        auto* itemSender = new QTableWidgetItem(sender);
        itemSender->setData(Qt::UserRole, msgId);
        ui->tableWidget->setItem(row, 0, itemSender);

        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(receiver));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(message));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(replied ? "是" : "否"));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(reply));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(sendTime));
        ui->tableWidget->setItem(row, 6, new QTableWidgetItem(replyTime));

        row++;
    }

    ui->message_total_label->setText(QString("消息总数：%1").arg(row));

    updateMonthChart(senderFilter, onlyNoReply);
}

void MessageAdminWindow::initChartsIfNeeded()
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

void MessageAdminWindow::updateMonthChart(const QString& senderFilter, bool onlyNoReply)
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

    QString sql = R"SQL(
        SELECT substr(send_time, 1, 7) AS ym, COUNT(*) AS cnt
        FROM messages
        WHERE receiver='admin'
    )SQL";

    if (!senderFilter.isEmpty()) {
        sql += " AND sender LIKE ? ";
    }
    if (onlyNoReply) {
        sql += " AND replied=0 ";
    }

    sql += R"SQL(
          AND substr(send_time, 1, 7) BETWEEN ? AND ?
        GROUP BY ym
        ORDER BY ym
    )SQL";

    QSqlQuery q(DbManager::instance().database());
    q.prepare(sql);

    if (!senderFilter.isEmpty()) {
        q.addBindValue("%" + senderFilter + "%");
    }
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


void MessageAdminWindow::onRefreshClicked()
{
    refreshData(false);
}

void MessageAdminWindow::onSearchClicked()
{
    refreshData(false);
}

void MessageAdminWindow::onNoReplyClicked()
{
    refreshData(true);
}

void MessageAdminWindow::onCellDoubleClicked(int row, int /*column*/)
{
    QTableWidgetItem* senderItem = ui->tableWidget->item(row, 0);
    if (!senderItem) return;

    const int msgId = senderItem->data(Qt::UserRole).toInt();
    const QString repliedText = ui->tableWidget->item(row, 3) ? ui->tableWidget->item(row, 3)->text() : "否";
    const bool alreadyReplied = (repliedText == "是");

    if (alreadyReplied) {
        QMessageBox::information(this, "已回复", "该消息已回复。\n你可以在表格中查看回复内容。");
        return;
    }

    ReplyWindow dlg(this);
    dlg.setHint("请输入回复内容（双击未回复消息可弹出此窗口）");
    if (dlg.exec() != QDialog::Accepted) return;

    QString err;
    if (!DbManager::instance().replyMessageById(msgId, dlg.getReplyText(), &err)) {
        QMessageBox::warning(this, "回复失败", err);
        return;
    }

    refreshData(false);
}
