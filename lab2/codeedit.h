#ifndef CODEEDIT_H
#define CODEEDIT_H

#include <QObject>
#include <QWidget>
#include <QPlainTextEdit>


class CodeEditor : public QPlainTextEdit
 {
     Q_OBJECT

 public:
     CodeEditor(QWidget *parent = 0);

     void lineNumberAreaPaintEvent(QPaintEvent *event);
     int lineNumberAreaWidth();

 protected:
     void resizeEvent(QResizeEvent *event) override;

 private slots:
     void updateLineNumberAreaWidth(int newBlockCount);
     void highlightCurrentLine();
     void updateLineNumberArea(const QRect &, int);

public slots:
     void hideLineNumberArea(bool flag);

 private:
     QWidget *lineNumberArea;
 };

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor) {
        codeEditor = editor;
    }

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};




#endif // CODEEDIT_H
