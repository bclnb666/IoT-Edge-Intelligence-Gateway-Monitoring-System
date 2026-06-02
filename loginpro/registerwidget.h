#ifndef REGISTERWIDGET_H
#define REGISTERWIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include "protocol.h"

namespace Ui {
class registerWidget;
}

class registerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit registerWidget(QWidget *parent = nullptr);
    ~registerWidget();

private slots:
    void on_pushButton_clicked();   // 点击注册按钮对应的槽函数
    void socketReadyReadSlot();     // 新增：处理服务器注册反馈的槽函数

private:
    Ui::registerWidget *ui;
    QTcpSocket *tcpSocket;          //用于通信的套接字
    struct register_st sendbuf;     //注册请求发送数据包缓存
};

#endif // REGISTERWIDGET_H