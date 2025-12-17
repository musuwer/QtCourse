#include "masterview.h"
#include "ui_masterview.h"
#include "idatabase.h"
#include <QDebug>

MasterView::MasterView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MasterView)
{
    ui->setupUi(this);

    // 去掉窗口边框（如果需要开启，取消注释）
    // this->setWindowFlag(Qt::FramelessWindowHint);

    // 初始化数据库单例，确保数据库连接建立
    IDatabase::getInstance();

    // 启动进入登录页面
    goLoginView();
}

MasterView::~MasterView()
{
    delete ui;
}

void MasterView::goLoginView()
{
    loginView = new LoginView(this);
    pushWidgetTOStackView(loginView);

    // 【优化】使用新的信号槽语法，编译期检查更安全
    connect(loginView, &LoginView::loginSucess, this, &MasterView::goWelcomView);
}

void MasterView::goWelcomView()
{
    welcomView = new WelcomeView(this);
    pushWidgetTOStackView(welcomView);

    // 【优化】使用新的信号槽语法
    connect(welcomView, &WelcomeView::goDoctorView, this, &MasterView::goDoctorView);
    connect(welcomView, &WelcomeView::goSectionView, this, &MasterView::goSectionView);
    connect(welcomView, &WelcomeView::goPatientView, this, &MasterView::goPatientView);
}

void MasterView::goDoctorView()
{
    doctorView = new DoctorView(this);
    pushWidgetTOStackView(doctorView);
}

void MasterView::goSectionView()
{
    sectionView = new SectionView(this);
    pushWidgetTOStackView(sectionView);
}

void MasterView::goPatientView()
{
    patientView = new PatientManage(this);
    pushWidgetTOStackView(patientView);

    connect(patientView, &PatientManage::goPatientEditView, this, &MasterView::goPatientEditView);
}

void MasterView::goPatientEditView(int rowNum)
{
    patientEditView1 = new patientEditView(this, rowNum);
    pushWidgetTOStackView(patientEditView1);

    connect(patientEditView1, &patientEditView::goPreviousView, this, &MasterView::goPreviousView);
}

void MasterView::goPreviousView()
{
    int count = ui->stackedWidget->count();

    if(count > 1){
        // 1. 获取当前页面（准备删除）
        QWidget *widgetToRemove = ui->stackedWidget->currentWidget();

        // 2. 切换到前一个页面
        ui->stackedWidget->setCurrentIndex(count - 2);

        // 3. 更新标题（获取新显示的页面的标题）
        if (ui->stackedWidget->currentWidget()) {
            ui->labelTitle->setText(ui->stackedWidget->currentWidget()->windowTitle());
        }

        // 4. 从栈中移除并释放内存
        ui->stackedWidget->removeWidget(widgetToRemove);
        if (widgetToRemove) {
            delete widgetToRemove;
            widgetToRemove = nullptr;
        }
    }
}

void MasterView::pushWidgetTOStackView(QWidget *widget)
{
    if (!widget) return;

    ui->stackedWidget->addWidget(widget);

    // 切换到最新添加的页面
    int count = ui->stackedWidget->count();
    ui->stackedWidget->setCurrentIndex(count - 1);

    // 设置标题
    ui->labelTitle->setText(widget->windowTitle());
}

void MasterView::on_btnBack_clicked()
{
    goPreviousView();
}

void MasterView::on_btnLogout_clicked()
{
    // 【重要修复】原来的 for 循环逻辑有误。
    // 因为在 removeWidget 后，count() 会变，会导致索引错乱或删不干净。
    // 正确的做法是：循环直到栈为空，然后重新进入登录页。

    int count = ui->stackedWidget->count();

    // 从后往前删，或者直接循环移除第一个，直到清空
    while(count > 0) {
        QWidget *widget = ui->stackedWidget->widget(count - 1); // 获取最后一个
        ui->stackedWidget->removeWidget(widget);
        delete widget;
        count = ui->stackedWidget->count(); // 更新数量
    }

    // 栈清空后，重新跳转到登录页
    goLoginView();
}

void MasterView::on_stackedWidget_currentChanged(int arg1)
{
    // 消除未引用参数警告
    Q_UNUSED(arg1);

    int count = ui->stackedWidget->count();

    // 只有一层页面时（通常是登录页），禁用返回按钮
    if(count > 1){
        ui->btnBack->setEnabled(true);
    }
    else {
        ui->btnBack->setEnabled(false);
    }

    // 获取当前页面标题，根据标题控制按钮状态
    // 注意：这里依赖 windowTitle 的字符串匹配，如果 UI 文件里修改了标题，这里也要改
    QWidget *currWidget = ui->stackedWidget->currentWidget();
    if (!currWidget) return;

    QString title = currWidget->windowTitle();

    if(title == "欢迎"){
        ui->btnBack->setEnabled(false);    // 欢迎页不能返回（因为前面是登录页，逻辑上不该回退到登录输入框）
        ui->btnLogout->setEnabled(true);   // 欢迎页可以注销
    }
    else if(title == "登录"){
        ui->btnBack->setEnabled(false);
        ui->btnLogout->setEnabled(false);  // 登录页不能注销
    }
    else {
        // 其他业务页面（如患者管理、医生管理）
        ui->btnLogout->setEnabled(false);  // 业务页面通常不直接注销，需返回欢迎页
        // 或者你可以允许在任何页面注销，看需求：
        // ui->btnLogout->setEnabled(true);
    }
}
