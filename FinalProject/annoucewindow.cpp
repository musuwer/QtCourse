#include "annoucewindow.h"
#include "ui_annouce_window.h"
#include "dbmanager.h"  // 引入数据库管理器
#include <QMessageBox>

AnnouceWindow::AnnouceWindow(const QString& author, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AnnouceWindow)
    , m_author(author) // 初始化发布者
{
    ui->setupUi(this);
    initUi();
    connectSignals();
}

AnnouceWindow::~AnnouceWindow()
{
    delete ui;
}

void AnnouceWindow::initUi()
{
    setWindowTitle("发布新公告");

    // 优化：设置窗口模态，防止用户在填写时点到后面的窗口
    setModal(true);

    // 优化：如果是 QDialog，通常去掉右上角的问号
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void AnnouceWindow::connectSignals()
{
    // 使用 Qt5/6 新语法连接信号槽（你原本写得就很好）
    connect(ui->send_pushButton, &QPushButton::clicked, this, &AnnouceWindow::onSendClicked);
    connect(ui->cancel_pushButton, &QPushButton::clicked, this, &AnnouceWindow::onCancelClicked);
}

void AnnouceWindow::onSendClicked()
{
    // 1. 获取输入并去空格
    QString title = getTitle();
    QString content = getContent();

    // 2. 校验输入
    if (title.isEmpty() || content.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "标题和内容不能为空，请输入后重试。");
        return;
    }

    // 3. 写入数据库 (核心优化)
    QString err;
    bool success = DbManager::instance().addAnnouncement(title, content, m_author, &err);

    if (success) {
        QMessageBox::information(this, "成功", "公告发布成功！");
        accept(); // 关闭窗口并返回 Accepted 状态
    } else {
        QMessageBox::critical(this, "数据库错误", "发布失败：" + err);
        // 失败时不关闭窗口，让用户有机会重试或复制内容
    }
}

void AnnouceWindow::onCancelClicked()
{
    reject(); // 关闭窗口并返回 Rejected 状态
}

QString AnnouceWindow::getTitle() const
{
    if (!ui) return QString();
    return ui->annouce_title_lineEdit->text().trimmed();
}

QString AnnouceWindow::getContent() const
{
    if (!ui) return QString();
    return ui->textEdit->toPlainText().trimmed();
}
