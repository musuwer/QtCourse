#include "patientmanage.h"
#include "ui_patientmanage.h"
#include "idatabase.h"
#include <QMessageBox> // 【新增】用于弹窗提示
#include <QDebug>

PatientManage::PatientManage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PatientManage)
{
    ui->setupUi(this);

    // 设置表格视图的属性
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);    // 选中整行
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);   // 只能单选
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);     // 禁止直接在表格内编辑
    ui->tableView->setAlternatingRowColors(true);                          // 隔行变色

    // 初始化模型
    IDatabase &iDatabase = IDatabase::getInstance();
    if(iDatabase.initPatientModel()){
        // 绑定数据模型和选择模型
        ui->tableView->setModel(iDatabase.patientTabModel);
        ui->tableView->setSelectionModel(iDatabase.patientSelection);
    }
    else {
        qDebug() << "PatientModel load failed!"; // 替换掉原来的 "123"
    }
}

PatientManage::~PatientManage()
{
    delete ui;
}

void PatientManage::on_btnSearch_clicked()
{
    QString input = ui->lineEdit->text().trimmed(); // 去除首尾空格
    QString filter = QString("NAME like '%%1%'").arg(input);
    IDatabase::getInstance().searchPatient(filter);
}

void PatientManage::on_btnAdd_clicked()
{
    // 调用数据库添加新行
    int currow = IDatabase::getInstance().addNewPatient();

    // 如果返回的行号有效，才跳转编辑
    if(currow != -1) {
        emit goPatientEditView(currow);
    } else {
        QMessageBox::critical(this, "错误", "创建新患者失败，请检查数据库连接。");
    }
}

void PatientManage::on_btnDelete_clicked()
{
    // 1. 获取当前选择
    QModelIndex curIndex = IDatabase::getInstance().patientSelection->currentIndex();

    // 2. 【健壮性检查】如果没有选中任何行，直接提示并返回
    if (!curIndex.isValid()) {
        QMessageBox::warning(this, "提示", "请先选择要删除的患者！");
        return;
    }

    // 3. 【交互优化】弹出确认对话框，防止误删
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  "确定要删除选中的患者信息吗？此操作无法恢复。",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool success = IDatabase::getInstance().deleteCurrentPatient();
        if (!success) {
            QMessageBox::warning(this, "错误", "删除失败，请稍后重试。");
        }
    }
}

void PatientManage::on_btnUpdate_clicked()
{
    // 1. 获取当前选择
    QModelIndex curIndex = IDatabase::getInstance().patientSelection->currentIndex();

    // 2. 【健壮性检查】防止未选中时点击修改导致程序异常
    if (!curIndex.isValid()) {
        QMessageBox::warning(this, "提示", "请先选择要修改的患者！");
        return;
    }

    // 3. 跳转到编辑页面
    emit goPatientEditView(curIndex.row());
}
