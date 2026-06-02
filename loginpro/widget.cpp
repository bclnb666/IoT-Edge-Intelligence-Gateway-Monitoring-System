#include "widget.h"
#include "ui_widget.h"
#include "protocol.h"
#include <QCryptographicHash>
#include <cstring>
#include <QMessageBox>
#include <QPalette>//包含QPalette类的头文件
#include "registerwidget.h"
#include "mainwidget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);




    ui->countEdit->setPlaceholderText(QString("输入账号"));
    ui->pwdEdit->setPlaceholderText(QString("输入密码"));
    ui->pwdEdit->setEchoMode(QLineEdit::Password);

    QPalette pal;//实例化QPalette类的对象
    pal.setBrush(QPalette::Window,QBrush(QPixmap(":/new/prefix1/imgs/background2.jpg")));
    this->setPalette(pal);

    connect(ui->loginButton, &QPushButton::clicked, this, &Widget::on_loginButton_clicked);

    tcpSocket = new QTcpSocket(this);

    connect(tcpSocket, &QTcpSocket::readyRead, this, &Widget::socketReadyReadSlot);

    connect(ui->registerButton, &QPushButton::clicked, this, &Widget::on_registerButton_clicked);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_loginButton_clicked()
{
    struct login_st sendbuf;

    //获取单行编辑组件输入的账号和密码
    QString cntstr = ui->countEdit->text();
    QString pwdstr = ui->pwdEdit->text();

    if(cntstr.isEmpty() || pwdstr.isEmpty())
    {
        QMessageBox::warning(this, QString("提示"), QString("用户名和密码不能为空！"), QMessageBox::StandardButtons(QMessageBox::Ok));
        return;
    }

    //将QString类型转换为char *
    QByteArray cntData = cntstr.toLatin1();
    strncpy(sendbuf.count, cntData.data(), CNTSIZE);
    //hash加密SHA512
    QByteArray pwdArray = QCryptographicHash::hash(pwdstr.toLatin1(), QCryptographicHash::Sha512);
    //先按16进制转字符，再转字符串
    QByteArray pwdHex = pwdArray.toHex();
    strncpy(sendbuf.passwd, pwdHex.data(), PWDSIZE);
    //指明当前为登录请求
    sendbuf.login_state = REQ_LOGIN;

    tcpSocket->connectToHost(QString(SERVER_IP),SERVER_PORT);
    tcpSocket->write((const char *)&sendbuf,sizeof(sendbuf));


}

void Widget::socketReadyReadSlot()
{
    struct login_st buf;
    tcpSocket->read((char *)&buf,sizeof(buf));
    switch (buf.login_state) {
    case LOGIN_SUCCESS:
    {
        QMessageBox::information(this,QString("登录状态"),QString("登录成功"),QMessageBox::StandardButtons(QMessageBox::Yes));
        this->hide(); // 隐藏登录窗体

        // 动态创建并打开主窗体
        mainWidget *mainWin = new mainWidget();
        mainWin->setAttribute(Qt::WA_DeleteOnClose); // 确保关闭时内存自动释放

        mainWin->show();
    }
        break;
    case LOGIN_ERROR_PASSWD:
        if(QMessageBox::question(this,QString("登录状态"),QString("登录密码错误！请重新输入！"),QMessageBox::StandardButtons(QMessageBox::Ok)) == QMessageBox::Ok)
        {
            ui->pwdEdit->clear();
            ui->pwdEdit->setFocus();
        }
        break;
    case LOGIN_COUNT_NO_EXIST:
        QMessageBox::information(this,QString("登录状态"),QString("登录账户不存在！"),QMessageBox::StandardButtons(QMessageBox::Ok));
        break;

    default:
        break;
    }

}

void Widget::on_registerButton_clicked()
{
    registerWidget *secondWidget = new registerWidget;
    // 设置窗口关闭时自动释放内存，防止重复打开注册页面产生内存泄漏
    secondWidget->setAttribute(Qt::WA_DeleteOnClose);
    secondWidget->show();
}

