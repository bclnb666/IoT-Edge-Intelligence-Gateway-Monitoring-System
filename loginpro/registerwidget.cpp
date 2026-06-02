#include "registerwidget.h"
#include "ui_registerwidget.h"
#include <QCryptographicHash>
#include <QMessageBox>
#include <cstring>

registerWidget::registerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::registerWidget)
{
    ui->setupUi(this);

    ui->lineEdit->setPlaceholderText(QString("输入您的账号"));
    ui->lineEdit_2->setPlaceholderText(QString("输入您的密码"));
    ui->lineEdit_2->setEchoMode(QLineEdit::Password);

    QPalette pal;//实例化QPalette类的对象
    pal.setBrush(QPalette::Window,QBrush(QPixmap(":/new/prefix1/imgs/background2.jpg")));
    this->setPalette(pal);

    // 初始化 TCP 套接字
    tcpSocket = new QTcpSocket(this);

    // 连接套接字的 readyRead 信号到接收处理槽函数
    connect(tcpSocket, &QTcpSocket::readyRead, this, &registerWidget::socketReadyReadSlot);
}

registerWidget::~registerWidget()
{
    delete ui;
}

void registerWidget::on_pushButton_clicked()
{
    // 获取单行编辑组件输入的账号和密码
    QString cntstr = ui->lineEdit->text();
    QString pwdstr = ui->lineEdit_2->text();

    if(cntstr.isEmpty() || pwdstr.isEmpty())
    {
        QMessageBox::warning(this, QString("提示"), QString("用户名和密码不能为空！"), QMessageBox::Ok);
        return;
    }

    // 初始化/清空发送缓冲区
    memset(&sendbuf, 0, sizeof(sendbuf));

    // 将 QString 类型安全地拷贝转换为 char *
    QByteArray cntData = cntstr.toLatin1();
    strncpy(sendbuf.count, cntData.data(), CNTSIZE);

    // hash加密SHA512
    QByteArray pwdArray = QCryptographicHash::hash(pwdstr.toLatin1(), QCryptographicHash::Sha512);
    // 先按16进制转字符，再转字符串
    QByteArray pwdHex = pwdArray.toHex();
    strncpy(sendbuf.passwd, pwdHex.data(), PWDSIZE);

    //指明这是“注册请求”类型
    sendbuf.register_state = REQ_REGISTER;

    //连接服务器
    tcpSocket->connectToHost(QString(SERVER_IP), SERVER_PORT);

    //等待连接建立完成，防止连接未成功时写入失败
    if (tcpSocket->waitForConnected(3000))
    {
        tcpSocket->write((const char *)&sendbuf, sizeof(sendbuf));
    }
    else
    {
        QMessageBox::critical(this, QString("网络错误"), QString("连接服务器失败，请检查网络！"), QMessageBox::Ok);
    }
}

//接收并处理服务器返回的注册响应
void registerWidget::socketReadyReadSlot()
{
    struct register_st recvbuf;
    memset(&recvbuf, 0, sizeof(recvbuf));

    //读取服务器发回的数据
    tcpSocket->read((char *)&recvbuf, sizeof(recvbuf));

    switch (recvbuf.register_state) {
    case REGISTER_SUCCESS:
        QMessageBox::information(this, QString("注册成功"), QString("恭喜，账号注册成功！"), QMessageBox::Ok);
        this->close(); //注册成功后自动关闭当前注册子窗口
        break;
    case REGISTER_COUNT_EXIST:
        QMessageBox::warning(this, QString("注册失败"), QString("该账号已被注册，请尝试其他账号！"), QMessageBox::Ok);
        ui->lineEdit->clear();
        ui->lineEdit->setFocus();
        break;
    default:
        QMessageBox::critical(this, QString("注册失败"), QString("未知错误，请重试！"), QMessageBox::Ok);
        break;
    }

}