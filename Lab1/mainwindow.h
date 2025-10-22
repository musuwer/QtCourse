#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QMap>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 与原接口保持一致（名字不变）
    void display(QString str);
    int  find(QString str, QString str1);
    QString caculate();   // 内部将转为调用“应用待处理运算”
    void warning();

private:
    Ui::MainWindow *ui;

    // -------- 计算状态机 --------
    enum Operation { None, Add, Sub, Mul, Div };
    double   acc = 0.0;          // 累计值 / 上一次计算结果
    Operation pendingOp = None;  // 待执行运算
    bool     justEvaluated = false; // 刚按过 '=' 用于决定是否清空开始新输入

    // 文本状态（沿用你的变量名，便于最小改动）
    QString operand; // 当前正在输入的数字
    QString result;  // 用于显示/同步（不再混放表达式）

    // 键盘与按钮映射
    QMap<int, QPushButton*> digiteBtns;
    QMap<int, QPushButton*> opBtns;

    // 工具函数
    bool    applyPending();                 // 执行 pendingOp(acc, operand)，成功返回 true
    void    setPending(Operation op);       // 设置待运算
    static  QString formatNumber(double x); // 去尾零格式化

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void btnNumClicked();
    void btnPointClicked();
    void btnDelClicked();
    void btnCleanClicked();   // C
    void btn0Clicked();       // 兼容原有独立0键逻辑
    void binaryOperateClicked();
    void btnEqualClicked();
    void btnReserveClicked(); // 1/x
    void btnPowClicked();     // x^2
    void btnSqrtClicked();    // sqrt
    void btnPercentClicked(); // %
    void btnCeClicked();      // CE
    void btnPlusAndNeg();     // ±
};

#endif // CALCULATOR_H
