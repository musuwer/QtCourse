#include "searchdialog.h"
#include "ui_searchdialog.h"

#include <QDebug>
#include <QMessageBox>
#include <QPlainTextEdit>

searchDialog::searchDialog(QWidget *parent, QPlainTextEdit *textEdit)
    : QDialog(parent),
    ui(new Ui::searchDialog),
    pTextEdit(textEdit)
{
    ui->setupUi(this);

    // 没有文本编辑器时禁用“查找”按钮
    if (!pTextEdit) {
        ui->findNextButton->setEnabled(false);
    }
}

searchDialog::~searchDialog()
{
    delete ui;
}

void searchDialog::on_findNextButton_clicked()
{
    // 基本有效性检查
    if (!pTextEdit) {
        QMessageBox::warning(this, tr("查找"), tr("没有可查找的文本区域。"));
        return;
    }

    const QString target = ui->searchText->text();
    if (target.isEmpty()) {
        QMessageBox::warning(this, tr("查找"), tr("请输入查找内容！"));
        return;
    }

    const QString srcText = pTextEdit->toPlainText();
    if (srcText.isEmpty()) {
        QMessageBox::information(this, tr("查找"), tr("文档内容为空。"));
        return;
    }

    QTextCursor cursor = pTextEdit->textCursor();
    const Qt::CaseSensitivity cs =
        ui->checkBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    int index = -1;

    // 向下查找
    if (ui->raDownButton->isChecked()) {
        // 从当前选择后的位置开始查找
        int startPos = cursor.selectionEnd();
        if (startPos < 0 || startPos >= srcText.length()) {
            startPos = 0; // 超出范围时从头开始（相当于“从头查找”）
        }

        index = srcText.indexOf(target, startPos, cs);

        // 未找到时尝试从头到起始位置做“环绕查找”
        if (index == -1 && startPos > 0) {
            index = srcText.indexOf(target, 0, cs);
        }

        if (index == -1) {
            QMessageBox::information(this, tr("查找"),
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
        // 从当前选择前的位置开始向前查找
        int startPos = cursor.selectionStart() - 1;
        if (startPos < 0) {
            startPos = srcText.length() - 1; // 超出范围时从末尾开始（环绕）
        }

        index = srcText.lastIndexOf(target, startPos, cs);

        // 如果还没找到，尝试从末尾到起始位置之前再查一次
        if (index == -1 && startPos < srcText.length() - 1) {
            index = srcText.lastIndexOf(target, -1, cs);
        }

        if (index == -1) {
            QMessageBox::information(this, tr("查找"),
                                     tr("未找到 \"%1\"").arg(target));
            return;
        }

        cursor.setPosition(index);
        cursor.setPosition(index + target.length(), QTextCursor::KeepAnchor);
        pTextEdit->setTextCursor(cursor);
        return;
    }

    // 未选择方向
    QMessageBox::warning(this, tr("查找"), tr("请选择查找方向。"));
}

void searchDialog::on_cancelButton_clicked()
{
    accept();
}
