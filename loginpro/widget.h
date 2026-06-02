#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

public slots:
    void socketReadyReadSlot();

private slots:
    void on_loginButton_clicked();

    void on_registerButton_clicked();

private:
    Ui::Widget *ui;
    QTcpSocket *tcpSocket;
};
#endif // WIDGET_H
