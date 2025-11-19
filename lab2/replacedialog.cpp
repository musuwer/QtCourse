#include "replacedialog.h"
#include "ui_replacedialog.h"

#include <QDebug>
#include <QMessageBox>
#include <QPlainTextEdit>

replaceDialog::replaceDialog(QWidget *parent, QPlainTextEdit *textEdit)
    : QDialog(parent),
    ui(new Ui::replaceDialog),
    pTextEdit(textEdit)
{
    ui->setupUi(this);

    // 没有文本编辑器时禁用按钮
    if (!pTextEdit) {
        ui->findNextButton->setEnabled(false);
        ui->replaceButton->setEnabled(false);
        ui->replaceAllButton->setEnabled(false);
    }
}

replaceDialog::~replaceDialog()
{
    delete ui;
}

void replaceDialog::on_findNextButton_clicked()
{
    if (!pTextEdit) {
        QMessageBox::warning(this, tr("替换"), tr("没有可查找的文本区域。"));
        return;
    }

    const QString target = ui->searchText->text();
    if (target.isEmpty()) {
        QMessageBox::warning(this, tr("替换"), tr("请输入查找内容！"));
        return;
    }

    const QString srcText = pTextEdit->toPlainText();
    if (srcText.isEmpty()) {
        QMessageBox::information(this, tr("替换"), tr("文档内容为空。"));
        return;
    }

    QTextCursor cursor = pTextEdit->textCursor();
    const Qt::CaseSensitivity cs =
        ui->checkBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    int index = -1;

    // 向下查找
    if (ui->raDownButton->isChecked()) {
        int startPos = cursor.selectionEnd();
        if (startPos < 0 || startPos >= srcText.length()) {
            startPos = 0; // 超出范围时从头开始
        }

        index = srcText.indexOf(target, startPos, cs);

        // 简单“环绕查找”
        if (index == -1 && startPos > 0) {
            index = srcText.indexOf(target, 0, cs);
        }

        if (index == -1) {
            QMessageBox::information(this, tr("替换"),
                                     tr("未找到 \"%1\"").arg(target));
            return;
        }

        cursor.setPosition(index);
        cursor.setPosition(index + target.length(), QTextCursor::KeepAnchor);
        pTextEdit->setTextCursor(cursor);
        return;
    }

    // 向上查找
    if (ui->raUpButton->isChecked()) {
        int startPos = cursor.selectionStart() - 1;
        if (startPos < 0) {
            startPos = srcText.length() - 1; // 超出范围时从末尾开始
        }

        index = srcText.lastIndexOf(target, startPos, cs);

        // 简单“环绕查找”
        if (index == -1 && startPos < srcText.length() - 1) {
            index = srcText.lastIndexOf(target, -1, cs);
        }

        if (index == -1) {
            QMessageBox::information(this, tr("替换"),
                                     tr("未找到 \"%1\"").arg(target));
            return;
        }

        cursor.setPosition(index);
        cursor.setPosition(index + target.length(), QTextCursor::KeepAnchor);
        pTextEdit->setTextCursor(cursor);
        return;
    }

    // 未选择方向
    QMessageBox::warning(this, tr("替换"), tr("请选择查找方向。"));
}

void replaceDialog::on_replaceButton_clicked()
{
    if (!pTextEdit) {
        QMessageBox::warning(this, tr("替换"), tr("没有可替换的文本区域。"));
        return;
    }

    const QString target = ui->searchText->text();
    if (target.isEmpty()) {
        QMessageBox::warning(this, tr("替换"), tr("请输入查找内容！"));
        return;
    }

    // 先找到下一处
    on_findNextButton_clicked();

    QTextCursor cursor = pTextEdit->textCursor();
    const QString selected = cursor.selectedText();
    const QString replacement = ui->targetText->text(); // 可为空，表示删除

    // 只有当选中的文本就是目标串时才执行替换，避免误操作
    if (selected == target) {
        pTextEdit->insertPlainText(replacement);
    }
}

void replaceDialog::on_replaceAllButton_clicked()
{
    if (!pTextEdit) {
        QMessageBox::warning(this, tr("替换"), tr("没有可替换的文本区域。"));
        return;
    }

    const QString target = ui->searchText->text();
    if (target.isEmpty()) {
        QMessageBox::warning(this, tr("替换"), tr("请输入查找内容！"));
        return;
    }

    QString text = pTextEdit->toPlainText();
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("替换"), tr("文档内容为空。"));
        return;
    }

    const QString replacement = ui->targetText->text(); // 允许为空
    const Qt::CaseSensitivity cs =
        ui->checkBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    text.replace(target, replacement, cs);
    pTextEdit->setPlainText(text);
}

void replaceDialog::on_cancelButton_clicked()
{
    accept();
}
