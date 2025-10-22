#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QtGlobal>
#include <QDebug>
#include <cmath>


static inline QString stripZeros(QString s) {
    if (s.contains('.')) {
        while (s.endsWith('0')) s.chop(1);
        if (s.endsWith('.')) s.chop(1);
    }
    return s;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 数字键映射（顶部行/小键盘均会触发相同的 Qt::Key_0..9）
    digiteBtns = {
                  {Qt::Key_0, ui->btnNum0}, {Qt::Key_1, ui->btnNum1},
                  {Qt::Key_2, ui->btnNum2}, {Qt::Key_3, ui->btnNum3},
                  {Qt::Key_4, ui->btnNum4}, {Qt::Key_5, ui->btnNum5},
                  {Qt::Key_6, ui->btnNum6}, {Qt::Key_7, ui->btnNum7},
                  {Qt::Key_8, ui->btnNum8}, {Qt::Key_9, ui->btnNum9},
                  };

    // 操作键映射
    opBtns = {
        {Qt::Key_Plus,      ui->btnPlus},
        {Qt::Key_Minus,     ui->btnSub},
        {Qt::Key_Asterisk,  ui->btnMul},
        {Qt::Key_Slash,     ui->btnDiv},
        {Qt::Key_Enter,     ui->btnEq},
        {Qt::Key_Return,    ui->btnEq},
        {Qt::Key_Equal,     ui->btnEq},
        {Qt::Key_Backspace, ui->btnDel}
    };

    // 连接数字键
    for (auto *btn : digiteBtns.values()) {
        connect(btn, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    }

    // 小数点
    connect(ui->btnPoint, SIGNAL(clicked()), this, SLOT(btnPointClicked()));

    // 删除、清空
    connect(ui->btnDel, SIGNAL(clicked()), this, SLOT(btnDelClicked()));
    connect(ui->btnC,   SIGNAL(clicked()), this, SLOT(btnCleanClicked()));

    // 双目操作符
    connect(ui->btnPlus, SIGNAL(clicked()), this, SLOT(binaryOperateClicked()));
    connect(ui->btnSub,  SIGNAL(clicked()), this, SLOT(binaryOperateClicked()));
    connect(ui->btnMul,  SIGNAL(clicked()), this, SLOT(binaryOperateClicked()));
    connect(ui->btnDiv,  SIGNAL(clicked()), this, SLOT(binaryOperateClicked()));

    // 等于
    connect(ui->btnEq, SIGNAL(clicked()), this, SLOT(btnEqualClicked()));

    // 一元运算
    connect(ui->btnReserve, SIGNAL(clicked()), this, SLOT(btnReserveClicked()));
    connect(ui->btnPow,     SIGNAL(clicked()), this, SLOT(btnPowClicked()));
    connect(ui->btnSqrt,    SIGNAL(clicked()), this, SLOT(btnSqrtClicked()));
    connect(ui->btnPercent, SIGNAL(clicked()), this, SLOT(btnPercentClicked()));
    connect(ui->btnPlusAndNeg, SIGNAL(clicked()), this, SLOT(btnPlusAndNeg()));

    // CE
    connect(ui->btnCE, SIGNAL(clicked()), this, SLOT(btnCeClicked()));

    // 按钮配色
    ui->btnPercent->setStyleSheet("background-color: #f39c12;");
    ui->btnC->setStyleSheet("background-color: #f39c12;");
    ui->btnCE->setStyleSheet("background-color: #f39c12;");
    ui->btnReserve->setStyleSheet("background-color: #f39c12;");
    ui->btnDel->setStyleSheet("background-color: #f39c12;");
    ui->btnPow->setStyleSheet("background-color: #f39c12;");
    ui->btnSqrt->setStyleSheet("background-color: #f39c12;");
    ui->btnDiv->setStyleSheet("background-color: #f39c12;");
    ui->btnMul->setStyleSheet("background-color: #f39c12;");
    ui->btnSub->setStyleSheet("background-color: #f39c12;");
    ui->btnPlus->setStyleSheet("background-color: #f39c12;");
    ui->btnEq->setStyleSheet("background-color: #f39c12;");
    ui->btnPlusAndNeg->setStyleSheet("background-color: #3498db;");
    ui->btnPoint->setStyleSheet("background-color: #3498db;");

    ui->btnNum0->setStyleSheet("background-color: #2980b9;");
    ui->btnNum1->setStyleSheet("background-color: #2980b9;");
    ui->btnNum2->setStyleSheet("background-color: #2980b9;");
    ui->btnNum3->setStyleSheet("background-color: #2980b9;");
    ui->btnNum4->setStyleSheet("background-color: #2980b9;");
    ui->btnNum5->setStyleSheet("background-color: #2980b9;");
    ui->btnNum6->setStyleSheet("background-color: #2980b9;");
    ui->btnNum7->setStyleSheet("background-color: #2980b9;");
    ui->btnNum8->setStyleSheet("background-color: #2980b9;");
    ui->btnNum9->setStyleSheet("background-color: #2980b9;");


    // 初始状态
    operand.clear();
    result.clear();
    acc = 0.0;
    pendingOp = None;
    justEvaluated = false;
    display("0");
}

void MainWindow::display(QString str)
{
    ui->lineEdit->setText(str);
    if (auto *btn = qobject_cast<QPushButton*>(sender())) {
        ui->statusbar->showMessage(btn->text() + " clicked");
    } else {
        ui->statusbar->clearMessage();
    }
}

// 统计子串出现次数（保持原有接口）
int MainWindow::find(QString s, QString t)
{
    int count = 0, pos = 0;
    while ((pos = s.indexOf(t, pos)) != -1) { ++count; pos += t.length(); }
    return count;
}

// 统一数值格式化
QString MainWindow::formatNumber(double x)
{
    return stripZeros(QString::number(x, 'f', 12));
}

// 应用待处理运算 acc (op) operand
bool MainWindow::applyPending()
{
    if (pendingOp == None) return true;               // 无运算可做
    if (operand.isEmpty())  return true;              // 没有右操作数，忽略

    bool ok = false;
    double rhs = operand.toDouble(&ok);
    if (!ok) { warning(); return false; }

    switch (pendingOp) {
    case Add: acc = acc + rhs; break;
    case Sub: acc = acc - rhs; break;
    case Mul: acc = acc * rhs; break;
    case Div:
        if (rhs == 0.0) { warning(); display("error"); return false; }
        acc = acc / rhs; break;
    case None: default: break;
    }
    operand = formatNumber(acc);  // 结果回写到当前输入，便于连续按等号/继续一元运算
    result = operand;
    return true;
}

void MainWindow::setPending(Operation op)
{
    pendingOp = op;
    justEvaluated = false;
}

void MainWindow::btnNumClicked()
{
    QString digit = qobject_cast<QPushButton*>(sender())->text();
    if (justEvaluated && pendingOp == None) { // 刚算完且未设置新运算：开始新的输入
        acc = 0.0; operand.clear(); result.clear(); justEvaluated = false;
    }
    if (operand == "0") operand.clear(); // 去掉多余前导0
    operand += digit;
    result = operand;
    display(operand);
}

void MainWindow::btnPointClicked()
{
    if (justEvaluated && pendingOp == None) { // 刚算完重新开始
        acc = 0.0; operand.clear(); result.clear(); justEvaluated = false;
    }
    if (operand.isEmpty()) operand = "0";
    if (!operand.contains('.')) {
        operand += ".";
        result = operand;
        display(operand);
    } else {
        warning();
    }
}

void MainWindow::btnDelClicked()
{
    if (!operand.isEmpty()) {
        operand.chop(1);
        if (operand.isEmpty()) { display("0"); result.clear(); }
        else { result = operand; display(operand); }
    } else {
        warning();
    }
}

void MainWindow::btnCleanClicked() // C：全清
{
    operand.clear();
    result.clear();
    acc = 0.0;
    pendingOp = None;
    justEvaluated = false;
    display("0");
}

void MainWindow::btn0Clicked()
{
    // 与普通数字统一处理，避免多段逻辑分叉
    btnNumClicked();
}

void MainWindow::binaryOperateClicked()
{
    // 将按钮文本映射到 Operation
    QString opTxt = qobject_cast<QPushButton*>(sender())->text();
    Operation op = None;
    if (opTxt == "+") op = Add;
    else if (opTxt == "-") op = Sub;
    else if (opTxt.contains("×") || opTxt == "*") op = Mul;
    else if (opTxt.contains("÷") || opTxt == "/") op = Div;

    // 首次设置运算：把当前输入收进 acc
    if (pendingOp == None) {
        if (!operand.isEmpty()) {
            bool ok=false; acc = operand.toDouble(&ok);
            if (!ok) { warning(); return; }
        }
    } else {
        // 已有待运算，则先把前一段算完，实现链式运算
        if (!applyPending()) return;
    }

    // 准备下一段输入
    setPending(op);
    operand.clear();
    display(formatNumber(acc));
}

void MainWindow::btnEqualClicked()
{
    if (!applyPending()) return;
    // 等于后清除待运算，保留结果在 acc / operand
    setPending(None);
    justEvaluated = true;
    display(formatNumber(acc));
}

void MainWindow::btnReserveClicked() // 1/x
{
    QString *target = operand.isEmpty() ? &result : &operand;
    if (target->isEmpty()) { warning(); return; }
    bool ok=false; double x = target->toDouble(&ok);
    if (!ok || x == 0.0) { warning(); display("error"); return; }
    x = 1.0 / x;
    *target = formatNumber(x);
    if (pendingOp == None) acc = x;
    result = *target;
    justEvaluated = false;
    display(*target);
}

void MainWindow::btnPowClicked() // x^2
{
    QString *target = operand.isEmpty() ? &result : &operand;
    if (target->isEmpty()) { warning(); return; }
    bool ok=false; double x = target->toDouble(&ok);
    if (!ok) { warning(); return; }
    x = x * x;
    *target = formatNumber(x);
    if (pendingOp == None) acc = x;
    result = *target;
    justEvaluated = false;
    display(*target);
}

void MainWindow::btnSqrtClicked()
{
    QString *target = operand.isEmpty() ? &result : &operand;
    if (target->isEmpty()) { warning(); return; }
    bool ok=false; double x = target->toDouble(&ok);
    if (!ok || x < 0.0) { warning(); display("error"); return; }
    x = std::sqrt(x);
    *target = formatNumber(x);
    if (pendingOp == None) acc = x;
    result = *target;
    justEvaluated = false;
    display(*target);
}

void MainWindow::btnPercentClicked()
{
    // 行为：若有待运算，按 Windows 计算器语义，将 rhs := acc * rhs / 100
    // 否则就是简单除以 100
    bool ok=false;
    if (pendingOp != None) {
        double rhs = operand.isEmpty()? 0.0 : operand.toDouble(&ok);
        if (!ok) { warning(); return; }
        double x = acc * rhs / 100.0;
        operand = formatNumber(x);
        result  = operand;
        display(operand);
    } else {
        QString *target = operand.isEmpty()? &result : &operand;
        if (target->isEmpty()) { warning(); return; }
        double x = target->toDouble(&ok);
        if (!ok) { warning(); return; }
        x = x / 100.0;
        *target = formatNumber(x);
        result = *target;
        acc = x;
        display(*target);
    }
    justEvaluated = false;
}

void MainWindow::btnPlusAndNeg()
{
    QString *target = operand.isEmpty()? &result : &operand;
    if (target->isEmpty()) { warning(); return; }
    bool ok=false; double x = target->toDouble(&ok);
    if (!ok) { warning(); return; }
    x = -x;
    *target = formatNumber(x);
    if (pendingOp == None) acc = x;
    result = *target;
    display(*target);
    justEvaluated = false;
}

void MainWindow::btnCeClicked() // CE：仅清当前输入
{
    operand.clear();
    display("0");
    // acc / pendingOp 保持不变，便于继续输入右操作数
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 数字键
    for (auto key : digiteBtns.keys()) {
        if (event->key() == key) {
            digiteBtns[key]->animateClick();
            return;
        }
    }
    // 操作键
    for (auto key : opBtns.keys()) {
        if (event->key() == key) {
            opBtns[key]->animateClick();
            return;
        }
    }
    // 小数点
    if (event->key() == Qt::Key_Period || event->text() == ".") {
        ui->btnPoint->animateClick();
        return;
    }
}

void MainWindow::warning()
{
    QApplication::beep();
}

MainWindow::~MainWindow()
{
    delete ui;
}
