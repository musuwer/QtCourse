#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void isSaveMessageBox();
    void initLineWrap();
    void initToolBar();
    void initStatusBar();
    void initCursor();

    //重载关闭窗口按钮，如果关闭时有文件修改，则询问是否保存
    void closeEvent(QCloseEvent *event) override
    {
        //声明不使用 event
//        Q_UNUSED(event);
        if(textChanged){
            //询问是否保存
            int result = QMessageBox::question(this,"新建","是否保存？",QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if(result == QMessageBox::Yes){
                on_actionSave_triggered();
            }
            else if(result == QMessageBox::Cancel){
                event->ignore();
            }
        }
    }

private slots:
    void on_actionAbout_triggered();

    void on_actionFind_triggered();

    void on_actionReplace_triggered();

    void on_actionNew_triggered();

    void on_actionSave_triggered();

    void on_actionSaveAs_triggered();

    void on_actionUndo_triggered();

    void on_actionCut_triggered();

    void on_actionCopy_triggered();

    void on_actionPaste_triggered();

    void on_actionOpen_triggered();

    void on_textEdit_textChanged();

    void on_actionRedo_triggered();

    void on_textEdit_copyAvailable(bool b);

    void on_textEdit_redoAvailable(bool b);

    void on_textEdit_undoAvailable(bool b);

    void on_actionFontColor_triggered();

    void on_actionFontBackColor_triggered();

    void on_actionEditorBackColor_triggered();

    void on_actionWrap_triggered();

    void on_actionFont_triggered();

    void on_actionTollbar_triggered();

    void on_actionStatusbar_triggered();

    void on_actionExit_triggered();

    void on_actionSeletAll_triggered();

    void on_textEdit_cursorPositionChanged();


    void on_actionShowLineNumber_triggered(bool checked);

private:
    Ui::MainWindow *ui;

    QLabel statusCursorLabel;
    QLabel statusLabel;
    QString filePath;
    bool textChanged;
    bool toolEnable;
};
#endif // MAINWINDOW_H
