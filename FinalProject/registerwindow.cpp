#include "registerwindow.h"
#include "ui_register_window.h"

#include <QMessageBox>
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>
#include "dbmanager.h"

RegisterWindow::RegisterWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RegisterWindow)
{
    ui->setupUi(this);

    initFrameless();

    ui->lineEditPass->setEchoMode(QLineEdit::Password);
    ui->lineEditConfirm->setEchoMode(QLineEdit::Password);

    connect(ui->register_pushButton, &QPushButton::clicked, this, &RegisterWindow::handleRegister);
    connect(ui->return_pushButton, &QPushButton::clicked, this, &RegisterWindow::handleReturn);
}

void RegisterWindow::initFrameless()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    if (ui->frame_4) {
        ui->frame_4->installEventFilter(this);
    }
    if (ui->min_button) {
        connect(ui->min_button, &QPushButton::clicked, this, &QWidget::showMinimized);
    }
    if (ui->max_button) {
        connect(ui->max_button, &QPushButton::clicked, this, [this]() {
            isMaximized() ? showNormal() : showMaximized();
        });
    }
    if (ui->close_button) {
        connect(ui->close_button, &QPushButton::clicked, this, &QWidget::close);
    }
}

bool RegisterWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->frame_4) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto* e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::LeftButton) {
                m_dragging = true;
                m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            if (m_dragging && !isMaximized()) {
                auto* e = static_cast<QMouseEvent*>(event);
                move(e->globalPosition().toPoint() - m_dragOffset);
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            m_dragging = false;
            break;
        }
        case QEvent::MouseButtonDblClick: {
            isMaximized() ? showNormal() : showMaximized();
            return true;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

RegisterWindow::~RegisterWindow()
{
    delete ui;
}

void RegisterWindow::handleReturn()
{
    emit backToLoginRequested();
    close();
}

void RegisterWindow::handleRegister()
{
    const QString user = ui->lineEditUser->text().trimmed();
    const QString pass = ui->lineEditPass->text();
    const QString confirm = ui->lineEditConfirm->text();

    if (user.isEmpty() || pass.isEmpty() || confirm.isEmpty()) {
        QMessageBox::warning(this, "注册", "请填写完整信息。");
        return;
    }
    if (pass != confirm) {
        QMessageBox::warning(this, "注册", "两次输入的密码不一致。");
        return;
    }

    QString dbErr;
    if (!DbManager::instance().init(&dbErr)) {
        QMessageBox::critical(this, "注册", QString("数据库初始化失败：\n%1\n\n数据库路径：\n%2")
                                          .arg(dbErr, DbManager::instance().databasePath()));
        return;
    }

    QString err;
    // 这里默认注册普通用户即可（DbManager 内部也会强制 role 为 user）
    if (!DbManager::instance().registerUser(user, pass, &err)) {
        QMessageBox::warning(this, "注册", QString("注册失败：%1").arg(err));
        return;
    }

    QMessageBox::information(this, "注册", "注册成功，请返回登录。");
    emit backToLoginRequested();
    close();
}
