#include "logrecordwindow.h"
#include "ui_logrecord_window.h"

#include <QSqlTableModel>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QDate>

#include "dbmanager.h"
#include "addlogwindow.h"

namespace {

class LogFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit LogFilterProxyModel(QObject* parent=nullptr) : QSortFilterProxyModel(parent) {}

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        const QString kw = filterRegularExpression().pattern().trimmed();
        if (kw.isEmpty()) return true;

        // title/person/place/date/score 全字段模糊匹配
        const int cols = sourceModel()->columnCount();
        for (int c = 0; c < cols; ++c) {
            const QModelIndex idx = sourceModel()->index(source_row, c, source_parent);
            const QString v = sourceModel()->data(idx).toString();
            if (v.contains(kw, Qt::CaseInsensitive)) return true;
        }
        return false;
    }
};

static int safeToInt(const QString& s, int def)
{
    bool ok=false;
    int v=s.trimmed().toInt(&ok);
    return ok ? v : def;
}

}

LogRecordWindow::LogRecordWindow(int userId, const QString& username, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LogRecordWindow)
    , m_userId(userId)
    , m_username(username)
{
    ui->setupUi(this);
    initUi();
    connectSignals();
    refreshData();
}

LogRecordWindow::~LogRecordWindow()
{
    delete ui;
}

void LogRecordWindow::initUi()
{
    // tableView 基础设置
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    // model
    m_model = new QSqlTableModel(this, DbManager::instance().database());
    m_model->setTable("logs");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->setFilter(QString("user_id=%1").arg(m_userId));
    m_model->select();

    // headers
    // logs: id,user_id,title,person,place,log_date,mood_score,created_at
    m_model->setHeaderData(2, Qt::Horizontal, "事件标题");
    m_model->setHeaderData(3, Qt::Horizontal, "相关人物");
    m_model->setHeaderData(4, Qt::Horizontal, "地点/备注");
    m_model->setHeaderData(5, Qt::Horizontal, "日期");
    m_model->setHeaderData(6, Qt::Horizontal, "情绪评分");
    m_model->setHeaderData(7, Qt::Horizontal, "创建时间");

    m_proxy = new LogFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);

    ui->tableView->setModel(m_proxy);
    // 隐藏 id/user_id
    ui->tableView->hideColumn(0);
    ui->tableView->hideColumn(1);

    updateCountLabel();
}

void LogRecordWindow::connectSignals()
{
    connect(ui->add_log_pushButton, &QPushButton::clicked, this, &LogRecordWindow::onAddLogClicked);
    connect(ui->refresh_pushButton, &QPushButton::clicked, this, &LogRecordWindow::onRefreshClicked);
    connect(ui->search_log_pushButton, &QPushButton::clicked, this, &LogRecordWindow::onSearchClicked);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &LogRecordWindow::onTableContextMenuRequested);
}

void LogRecordWindow::refreshData()
{
    if (!m_model) return;
    m_model->setFilter(QString("user_id=%1").arg(m_userId));
    m_model->select();
    updateCountLabel();
}

void LogRecordWindow::updateCountLabel()
{
    if (!m_model) return;
    ui->book_total_label->setText(QString("总计：%1").arg(m_model->rowCount()));
}

void LogRecordWindow::onAddLogClicked()
{
    AddLogWindow dlg(this);
    dlg.setWindowTitle("添加事件记录");

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const QString title = dlg.getTitle();
    const QString person = dlg.getPerson();
    const QString place = dlg.getPlace();
    QString dateStr = dlg.getDateStr();
    int score = dlg.getMoodScore();

    if (dateStr.trimmed().isEmpty()) {
        dateStr = QDate::currentDate().toString("yyyy-MM-dd");
    } else {
        // 尝试规范化为 yyyy-MM-dd
        const QDate d = QDate::fromString(dateStr.trimmed(), "yyyy-MM-dd");
        if (d.isValid()) dateStr = d.toString("yyyy-MM-dd");
    }

    QString err;
    if (!DbManager::instance().addLog(m_userId, title, person, place, dateStr, score, &err)) {
        QMessageBox::warning(this, "添加失败", err);
        return;
    }

    refreshData();
}

void LogRecordWindow::onRefreshClicked()
{
    ui->log_search_content_lineEdit->clear();
    m_proxy->setFilterRegularExpression(QRegularExpression());
    refreshData();
}

void LogRecordWindow::onSearchClicked()
{
    const QString kw = ui->log_search_content_lineEdit->text().trimmed();
    if (kw.isEmpty()) {
        m_proxy->setFilterRegularExpression(QRegularExpression());
    } else {
        // 直接用 pattern 存关键字，LogFilterProxyModel 会按 contains 来匹配
        m_proxy->setFilterRegularExpression(QRegularExpression(kw, QRegularExpression::CaseInsensitiveOption));
    }
}

void LogRecordWindow::onTableContextMenuRequested(const QPoint& pos)
{
    const QModelIndex proxyIdx = ui->tableView->indexAt(pos);
    if (!proxyIdx.isValid()) return;

    QMenu menu(this);
    QAction* actDelete = menu.addAction("删除选中记录");
    QAction* chosen = menu.exec(ui->tableView->viewport()->mapToGlobal(pos));
    if (chosen != actDelete) return;

    const QModelIndex srcIdx = m_proxy->mapToSource(proxyIdx);
    const int row = srcIdx.row();
    const int logId = m_model->data(m_model->index(row, 0)).toInt();

    const auto ret = QMessageBox::question(this, "确认删除", "确定删除该记录吗？");
    if (ret != QMessageBox::Yes) return;

    QString err;
    if (!DbManager::instance().deleteLogById(logId, &err)) {
        QMessageBox::warning(this, "删除失败", err);
        return;
    }
    refreshData();
}
