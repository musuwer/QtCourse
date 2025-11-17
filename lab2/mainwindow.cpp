#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "codeedit.h"
#include <aboutdialog.h>
#include <searchdialog.h>
#include <replacedialog.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDebug>
#include <QColorDialog>
#include <QFontDialog>
#include <QPlainTextEdit>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("新建文本文档");

    statusLabel.setMaximumWidth(200);
    statusLabel.setText("length: "+QString::number(1)+"   lines: "+QString::number(1));
    ui->statusbar->addPermanentWidget(&statusLabel);
    statusCursorLabel.setMaximumWidth(200);
    statusCursorLabel.setText("Ln: "+QString::number(1)+"   Col: "+QString::number(1));
    ui->statusbar->addPermanentWidget(&statusCursorLabel);

    QLabel *author = new QLabel(ui->statusbar);
    author->setText("  皮昊旋  ");
    ui->statusbar->addPermanentWidget(author);

    textChanged = false;
    ui->actionCopy->setEnabled(false);
    ui->actionCut->setEnabled(false);
    ui->actionUndo->setEnabled(false);
    ui->actionRedo->setEnabled(false);
    ui->actionPaste->setEnabled(false);

    //初始化自动换行状态
    initLineWrap();

    //初始化工具栏状态
    initToolBar();

    //初始化字体和字号
    QFont initFont("Arial",28);
    ui->textEdit->setFont(initFont);

    ui->textEdit->hideLineNumberArea(true);
    ui->actionShowLineNumber->setChecked(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::isSaveMessageBox()
{
    //询问是否保存
    int result = QMessageBox::question(this,"新建","是否保存？",QMessageBox::Yes | QMessageBox::No);
    if(result == QMessageBox::Yes){
        on_actionSave_triggered();
    }
}

void MainWindow::initLineWrap()
{
    QPlainTextEdit::LineWrapMode mode =  ui->textEdit->lineWrapMode();
    if(mode == QPlainTextEdit::NoWrap){
        ui->textEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        ui->actionWrap->setChecked(true);
    } else {
        ui->textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        ui->actionWrap->setChecked(false);
    }
}

void MainWindow::initToolBar()
{
    ui->toolBar->show();
    ui->actionTollbar->setChecked(false);
}

void MainWindow::initStatusBar()
{
    ui->statusbar->show();
    ui->actionStatusbar->setChecked(false);
}

void MainWindow::initCursor()
{
    QTextCursor cursor = ui->textEdit->textCursor();

    // 设置光标位置为文本的开头
    cursor.setPosition(0);

    // 将光标应用到文本编辑框
    ui->textEdit->setTextCursor(cursor);
}


void MainWindow::on_actionAbout_triggered()
{
    aboutDialog Adlg;
    Adlg.exec();
}


void MainWindow::on_actionFind_triggered()
{
    searchDialog Sdlg(this,ui->textEdit);
    Sdlg.exec();
}


void MainWindow::on_actionReplace_triggered()
{
    replaceDialog Rdlg(this,ui->textEdit);
    Rdlg.exec();
}


void MainWindow::on_actionNew_triggered()
{
    //询问是否保存
    if(textChanged){
        isSaveMessageBox();
    }


    //如果不清空，上一次保存玩文件 后直接新建再保存会覆盖上一次的文件
    filePath.clear();

    //初始化自动换行状态
    initLineWrap();

    ui->textEdit->clear();
    this->setWindowTitle("新建文本文档");

    //新建后text部分会清空，所以标记为以改变
    textChanged = false;

}


void MainWindow::on_actionOpen_triggered()
{

    //询问是否保存
    if(textChanged){
        isSaveMessageBox();
    }
    //如果连续打开两个文件，不清屏，第二个文件会加在第一个文件之后
    //在清屏前设置为true状态，防止用户一直点open但是不打开文件，导致标头前面好多*号   v
    textChanged = true;
    ui->textEdit->clear();
    textChanged = false;

    //初始化自动换行的状态
    initLineWrap();

    QString filename = QFileDialog::getOpenFileName(this,"打开文件",".",tr("Text files (*.txt);;All(*.*)"));
    QFile file(filename);

    //如果未选择，直接返回，不需要弹出错误消息
    if(filename.isEmpty())
    {
        return;
    }

    if(!file.open(QFile::ReadOnly | QFile::Text)){
        QMessageBox::warning(this,"WARNING","打开文件失败");
        return;
    }

    filePath = filename;

    QTextStream in(&file);
    QString text = in.readAll();
    ui->textEdit->insertPlainText(text);
    file.close();

    //初始化光标位置为第一个
    initCursor();

    this->setWindowTitle(QFileInfo(filename).absoluteFilePath());
    textChanged = false;
}



void MainWindow::on_actionSave_triggered()
{
    QFile file(filePath);
    if(!file.open(QFile::WriteOnly | QFile::Text)){

        //把前一个失败的file关闭
        file.close();

        QString filename = QFileDialog::getSaveFileName(this,"保存文件",".",tr("Text Files (*.txt)"));
        QFile file(filename);
        //用户直接关闭，不保存了
        if(filename.isEmpty()) return;
        if(!file.open(QFile::WriteOnly | QFile::Text)){
            QMessageBox::warning(this,"WARNING","打开文件失败");
            return;
        }
        filePath = filename;
    }

    QTextStream out(&file);
    QString text = ui->textEdit->toPlainText();
    out<<text;
    file.flush();
    file.close();

    //标记已保存
    this->setWindowTitle(QFileInfo(filePath).absoluteFilePath()+" 已保存");
    textChanged = false;
}


void MainWindow::on_actionSaveAs_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,"保存文件",".",tr("Text Files (*.txt)"));

    QFile file(filename);

    //用户直接关闭，不保存了
    if(filename.isEmpty()) return;

    if(!file.open(QFile::WriteOnly | QFile::Text)){

        if(!file.open(QFile::WriteOnly | QFile::Text)){
            QMessageBox::warning(this,"WARNING","打开文件失败");
            return;
        }
        filePath = filename;
    }


    QTextStream out(&file);
    QString text = ui->textEdit->toPlainText();
    out<<text;
    file.flush();
    file.close();

    //标记已保存
    this->setWindowTitle(QFileInfo(filename).absoluteFilePath()+" 已保存");
    textChanged = false;

}


void MainWindow::on_textEdit_undoAvailable(bool b)
{
    ui->actionUndo->setEnabled(b);
}


void MainWindow::on_actionUndo_triggered()
{
    ui->textEdit->undo();

}


void MainWindow::on_actionCut_triggered()
{
    ui->textEdit->cut();
    ui->actionPaste->setEnabled(true);
}


void MainWindow::on_textEdit_copyAvailable(bool b)
{
    ui->actionCopy->setEnabled(b);
    ui->actionCut->setEnabled(b);
}


void MainWindow::on_actionCopy_triggered()
{
    ui->textEdit->copy();
    ui->actionPaste->setEnabled(true);
}


void MainWindow::on_actionPaste_triggered()
{
    ui->textEdit->paste();
}


void MainWindow::on_textEdit_redoAvailable(bool b)
{
    ui->actionRedo->setEnabled(b);
}


void MainWindow::on_actionRedo_triggered()
{
    ui->textEdit->redo();
}


void MainWindow::on_textEdit_textChanged()
{
    if(!textChanged){
        this->setWindowTitle("*"+this->windowTitle());
        textChanged = true;
    }
    statusLabel.setText("length: "+QString::number(ui->textEdit->toPlainText().length())+"   lines: "+QString::number(ui->textEdit->document()->lineCount()));
}


void MainWindow::on_actionFontColor_triggered()
{
    QColor color = QColorDialog::getColor(Qt::black, this, "选择字体颜色");

    if (color.isValid()) {
        // 获取选定的颜色并将其应用于文本编辑框
        QPalette palette = ui->textEdit->palette();
        palette.setColor(QPalette::Text, color);
        ui->textEdit->setPalette(palette);
    }
}



void MainWindow::on_actionFontBackColor_triggered()
{

}


void MainWindow::on_actionEditorBackColor_triggered()
{
    QColor color = QColorDialog::getColor(Qt::black, this, "选择编辑器背景颜色");
    if(color.isValid()){

        QPalette palette = ui->textEdit->palette();
        palette.setColor(QPalette::Base, color);
        ui->textEdit->setPalette(palette);
    }
}


void MainWindow::on_actionWrap_triggered()
{
    QPlainTextEdit::LineWrapMode mode =  ui->textEdit->lineWrapMode();
    if(mode == QPlainTextEdit::NoWrap){
        ui->textEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        ui->actionWrap->setChecked(true);
    } else {
        ui->textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        ui->actionWrap->setChecked(false);
    }
}



void MainWindow::on_actionFont_triggered()
{
    bool ok = false;
    QFont font = QFontDialog::getFont(&ok,this);
    if(ok){
        ui->textEdit->setFont(font);
    }

}


void MainWindow::on_actionTollbar_triggered()
{
    bool isHidden = ui->toolBar->isHidden();
    if(isHidden){
        ui->actionTollbar->setChecked(false);
        ui->toolBar->show();
    }
    else{
        ui->actionTollbar->setChecked(true);
        ui->toolBar->hide();
    }
}


void MainWindow::on_actionStatusbar_triggered()
{
    bool isHidden = ui->statusbar->isHidden();
    if(isHidden){
        ui->actionStatusbar->setChecked(false);
        ui->statusbar->show();
    }
    else{
        ui->actionStatusbar->setChecked(true);
        ui->statusbar->hide();
    }
}


void MainWindow::on_actionExit_triggered()
{
    if(textChanged) isSaveMessageBox();
    exit(0);
}


void MainWindow::on_actionSeletAll_triggered()
{
    ui->textEdit->selectAll();
}


void MainWindow::on_textEdit_cursorPositionChanged()
{
    int ln = 1;

    //获取当前光标在文本中的位置
    int pos = ui->textEdit->textCursor().position();
    QString text = ui->textEdit->toPlainText();

    //统计行数
    for(int  i = 0;i<pos;i++){
        if(text[i]=='\n'){
            ln++;
        }
    }

    statusCursorLabel.setText("Ln: "+QString::number(ln)+"   Col: "+QString::number(pos+1));
}


void MainWindow::on_actionShowLineNumber_triggered(bool checked)
{
    //选中了就不隐藏
    ui->textEdit->hideLineNumberArea(!checked);
}

