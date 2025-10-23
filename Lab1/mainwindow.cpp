#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QtGlobal>
#include <QDebug>
#include <cmath>
#include <QApplication>
#include <QKeyEvent>

// 常量定义：限制最大输入长度（避免超长字符串导致异常）
const int MAX_INPUT_LENGTH = 20;

static inline QString stripZeros(QString s) {
    if (s.isEmpty()) return "0";
    if (s.contains('.')) {
        while (s.endsWith('0')) s.chop(1);
        if (s.endsWith('.')) s.chop(1);
    }
    if (s == ".") return "0";
    return s;
}

void MainWindow::setPending(Operation op)
{
    pendingOp = op;
    justEvaluated = false;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 数字键映射
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
        connect(btn, &QPushButton::clicked, this, &MainWindow::btnNumClicked);
    }

    // 连接其他按钮
    connect(ui->btnPoint, &QPushButton::clicked, this, &MainWindow::btnPointClicked);
    connect(ui->btnDel, &QPushButton::clicked, this, &MainWindow::btnDelClicked);
    connect(ui->btnC, &QPushButton::clicked, this, &MainWindow::btnCleanClicked);
    connect(ui->btnPlus, &QPushButton::clicked, this, &MainWindow::binaryOperateClicked);
    connect(ui->btnSub, &QPushButton::clicked, this, &MainWindow::binaryOperateClicked);
    connect(ui->btnMul, &QPushButton::clicked, this, &MainWindow::binaryOperateClicked);
    connect(ui->btnDiv, &QPushButton::clicked, this, &MainWindow::binaryOperateClicked);
    connect(ui->btnEq, &QPushButton::clicked, this, &MainWindow::btnEqualClicked);
    connect(ui->btnReserve, &QPushButton::clicked, this, &MainWindow::btnReserveClicked);
    connect(ui->btnPow, &QPushButton::clicked, this, &MainWindow::btnPowClicked);
    connect(ui->btnSqrt, &QPushButton::clicked, this, &MainWindow::btnSqrtClicked);
    connect(ui->btnPercent, &QPushButton::clicked, this, &MainWindow::btnPercentClicked);
    connect(ui->btnPlusAndNeg, &QPushButton::clicked, this, &MainWindow::btnPlusAndNeg);
    connect(ui->btnCE, &QPushButton::clicked, this, &MainWindow::btnCeClicked);

    // 按钮配色
    QList<QPushButton*> funcButtons = {
        ui->btnPercent, ui->btnC, ui->btnCE, ui->btnReserve,
        ui->btnDel, ui->btnPow, ui->btnSqrt, ui->btnDiv,
        ui->btnMul, ui->btnSub, ui->btnPlus, ui->btnEq
    };
    for (auto *btn : funcButtons) {
        btn->setStyleSheet("background-color: #f39c12;");
    }
    ui->btnPlusAndNeg->setStyleSheet("background-color: #3498db;");
    ui->btnPoint->setStyleSheet("background-color: #3498db;");

    QList<QPushButton*> numButtons = {
        ui->btnNum0, ui->btnNum1, ui->btnNum2, ui->btnNum3,
        ui->btnNum4, ui->btnNum5, ui->btnNum6, ui->btnNum7,
        ui->btnNum8, ui->btnNum9
    };
    for (auto *btn : numButtons) {
        btn->setStyleSheet("background-color: #2980b9;");
    }

    // 初始状态（增强：显式初始化所有变量）
    operand.clear();
    result.clear();
    acc = 0.0;
    pendingOp = None;
    justEvaluated = false;
    inErrorState = false; // 新增：错误状态标记
    display("0");
}

// 显示函数（增强：处理错误状态显示，限制长度）
void MainWindow::display(QString str)
{
    // 限制显示长度，避免UI异常
    if (str.length() > MAX_INPUT_LENGTH) {
        str = str.left(MAX_INPUT_LENGTH) + "...";
    }
    ui->lineEdit->setText(str);
    if (auto *btn = qobject_cast<QPushButton*>(sender())) {
        ui->statusbar->showMessage(btn->text() + " clicked");
    } else {
        ui->statusbar->clearMessage();
    }
}

// 统一数值格式化（增强：处理inf/nan等异常值）
QString MainWindow::formatNumber(double x)
{
    // 处理数值溢出或无效值
    if (std::isinf(x)) {
        inErrorState = true;
        return "overflow";
    }
    if (std::isnan(x)) {
        inErrorState = true;
        return "error";
    }
    // 处理极大/极小值的科学计数法显示（避免字符串过长）
    if (qAbs(x) > 1e15 || qAbs(x) < 1e-10) {
        return stripZeros(QString::number(x, 'e', 6)); // 科学计数法
    }
    return stripZeros(QString::number(x, 'f', 12));
}


// 应用待处理运算（增强：状态校验，错误恢复）
bool MainWindow::applyPending()
{
    if (inErrorState) { // 错误状态下不执行运算
        return false;
    }
    if (pendingOp == None) return true;
    if (operand.isEmpty()) return true;

    bool ok = false;
    double rhs = operand.toDouble(&ok);
    if (!ok) {
        warning();
        inErrorState = true;
        display("error");
        return false;
    }

    // 运算前检查acc是否有效
    if (std::isinf(acc) || std::isnan(acc)) {
        warning();
        inErrorState = true;
        display("error");
        return false;
    }

    switch (pendingOp) {
    case Add: acc += rhs; break;
    case Sub: acc -= rhs; break;
    case Mul: acc *= rhs; break;
    case Div:
        if (qFuzzyCompare(rhs, 0.0)) { // 用模糊比较处理浮点数精度问题
            warning();
            inErrorState = true;
            display("error: div0");
            return false;
        }
        acc /= rhs;
        break;
    default: break;
    }

    operand = formatNumber(acc);
    result = operand;
    inErrorState = false; // 运算成功，退出错误状态
    return true;
}

// 数字键点击（增强：错误状态重置，输入长度限制）
void MainWindow::btnNumClicked()
{
    // 错误状态下点击数字键，重置计算器
    if (inErrorState) {
        btnCleanClicked();
    }

    QString digit = qobject_cast<QPushButton*>(sender())->text();
    if (justEvaluated && pendingOp == None) {
        acc = 0.0;
        operand.clear();
        result.clear();
        justEvaluated = false;
    }

    // 限制最大输入长度（避免超长字符串）
    if (operand.length() >= MAX_INPUT_LENGTH) {
        warning();
        return;
    }

    if (operand == "0") operand.clear();
    operand += digit;
    result = operand;
    display(operand);
}

// 小数点处理（增强：错误状态处理）
void MainWindow::btnPointClicked()
{
    if (inErrorState) { // 错误状态下重置
        btnCleanClicked();
    }

    if (justEvaluated && pendingOp == None) {
        acc = 0.0;
        operand.clear();
        result.clear();
        justEvaluated = false;
    }

    if (operand.isEmpty()) operand = "0";
    if (!operand.contains('.') && operand.length() < MAX_INPUT_LENGTH - 1) { // 预留小数点位置
        operand += ".";
        result = operand;
        display(operand);
    } else {
        warning();
    }
}

// 退格操作（增强：错误状态处理）
void MainWindow::btnDelClicked()
{
    if (inErrorState) { // 错误状态下退格重置
        btnCleanClicked();
        return;
    }

    if (!operand.isEmpty()) {
        operand.chop(1);
        if (operand.isEmpty()) {
            display("0");
            result.clear();
        } else {
            result = operand;
            display(operand);
        }
    } else {
        warning();
    }
}

// 全清操作（增强：显式重置所有状态）
void MainWindow::btnCleanClicked()
{
    operand.clear();
    result.clear();
    acc = 0.0;
    pendingOp = None;
    justEvaluated = false;
    inErrorState = false; // 重置错误状态
    display("0");
}

// 双目操作符处理（增强：连续操作符更新，错误状态处理）
void MainWindow::binaryOperateClicked()
{
    if (inErrorState) { // 错误状态下不响应
        return;
    }

    QString opTxt = qobject_cast<QPushButton*>(sender())->text();
    Operation op = None;
    if (opTxt == "+") op = Add;
    else if (opTxt == "-") op = Sub;
    else if (opTxt.contains("×") || opTxt == "*") op = Mul;
    else if (opTxt.contains("÷") || opTxt == "/") op = Div;

    // 连续点击操作符时，更新为最后一个操作符（而非警告）
    if (pendingOp != None && operand.isEmpty()) {
        pendingOp = op;
        return;
    }

    if (pendingOp == None) {
        if (!operand.isEmpty()) {
            bool ok = false;
            acc = operand.toDouble(&ok);
            if (!ok) {
                warning();
                inErrorState = true;
                display("error");
                return;
            }
        } else {
            // 未输入数字直接点击操作符：将acc设为0（如直接"+5"→0+5）
            acc = 0.0;
        }
    } else {
        if (!applyPending()) return;
    }

    setPending(op);
    operand.clear();
    display(formatNumber(acc));
}

// 等号处理（增强：错误状态下不执行）
void MainWindow::btnEqualClicked()
{
    if (inErrorState) {
        return;
    }
    if (!applyPending()) return;
    setPending(None);
    justEvaluated = true;
    display(formatNumber(acc));
}

// 倒数（增强：错误状态处理）
void MainWindow::btnReserveClicked()
{
    if (inErrorState) {
        btnCleanClicked();
        return;
    }

    QString *target = operand.isEmpty() ? &result : &operand;
    if (target->isEmpty()) {
        warning();
        return;
    }

    bool ok = false;
    double x = target->toDouble(&ok);
    if (!ok || qFuzzyCompare(x, 0.0)) {
        warning();
        inErrorState = true;
        display("error");
        return;
    }

    x = 1.0 / x;
    *target = formatNumber(x);
    if (pendingOp == None) acc = x;
    result = *target;
    justEvaluated = false;
    display(*target);
}

// 平方（增强：错误状态处理）
void MainWindow::btnPowClicked()
{
    if (inErrorState) {
        btnCleanClicked();
        return;
    }

    QString *target = operand.isEmpty() ? &result : &operand;
    if (target->isEmpty()) {
        warning();
        return;
    }

    bool ok = false;
    double x = target->toDouble(&ok);
    if (!ok) {
        warning();
        return;
    }

    x = x * x;
    *target = formatNumber(x);
    if (pendingOp == None) acc = x;
    result = *target;
    justEvaluated = false;
    display(*target);
}

// 开方（增强：错误状态处理）
void MainWindow::btnSqrtClicked()
{
    if (inErrorState) {
        btnCleanClicked();
        return;
    }

    QString *target = operand.isEmpty() ? &result : &operand;
    if (target->isEmpty()) {
        warning();
        return;
    }

    bool ok = false;
    double x = target->toDouble(&ok);
    if (!ok || x < 0.0) {
        warning();
        inErrorState = true;
        display("error");
        return;
    }

    x = std::sqrt(x);
    *target = formatNumber(x);
    if (pendingOp == None) acc = x;
    result = *target;
    justEvaluated = false;
    display(*target);
}

// 百分比（增强：错误状态处理）
void MainWindow::btnPercentClicked()
{
    if (inErrorState) {
        btnCleanClicked();
        return;
    }

    bool ok = false;
    if (pendingOp != None) {
        double rhs = operand.isEmpty() ? 0.0 : operand.toDouble(&ok);
        if (!ok) {
            warning();
            return;
        }
        double x = acc * rhs / 100.0;
        operand = formatNumber(x);
        result = operand;
        display(operand);
    } else {
        QString *target = operand.isEmpty() ? &result : &operand;
        if (target->isEmpty()) {
            warning();
            return;
        }
        double x = target->toDouble(&ok);
        if (!ok) {
            warning();
            return;
        }
        x = x / 100.0;
        *target = formatNumber(x);
        result = *target;
        acc = x;
        display(*target);
    }
    justEvaluated = false;
}

// 正负号（增强：错误状态处理）
void MainWindow::btnPlusAndNeg()
{
    if (inErrorState) {
        btnCleanClicked();
        return;
    }

    QString *target = operand.isEmpty() ? &result : &operand;
    if (target->isEmpty()) {
        warning();
        return;
    }

    bool ok = false;
    double x = target->toDouble(&ok);
    if (!ok) {
        warning();
        return;
    }

    x = -x;
    *target = formatNumber(x);
    if (pendingOp == None) acc = x;
    result = *target;
    display(*target);
    justEvaluated = false;
}

// CE清除当前输入（增强：错误状态处理）
void MainWindow::btnCeClicked()
{
    if (inErrorState) {
        btnCleanClicked();
        return;
    }
    operand.clear();
    display("0");
}

// 键盘事件（增强：过滤无效键，错误状态下重置）
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (inErrorState) {
        // 错误状态下按任意数字键重置
        if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9) {
            btnCleanClicked();
        } else {
            return; // 其他键不响应
        }
    }

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

    // 忽略其他无效键（避免意外触发）
    event->ignore();
}

// 警告函数（增强：错误状态下不重复警告）
void MainWindow::warning()
{
    if (!inErrorState) { // 错误状态下只警告一次
        QApplication::beep();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
