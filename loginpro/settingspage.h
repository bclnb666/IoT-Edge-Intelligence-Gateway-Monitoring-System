#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include "protocol.h"

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

    // embsky 配置访问器
    EmbskyConfig embskyConfig() const;

    void updateEmbskyConnectionStatus(bool connected);

signals:
    void embskyConnectRequested(const EmbskyConfig &config);
    void embskyDisconnectRequested();

private slots:
    void onConnectClicked();

private:
    // ========== embsky 配置控件 ==========
    QLineEdit *m_txtEmbskyHost;
    QSpinBox  *m_spinEmbskyPort;
    QLineEdit *m_txtEmbskyUsername;
    QLineEdit *m_txtEmbskyPassword;
    QLineEdit *m_txtEmbskyClientId;
    QLineEdit *m_txtTempTopic;
    QLineEdit *m_txtHumiTopic;

    // ========== 公共控件 ==========
    QPushButton *m_btnConnect;
    QLabel *m_lblConnStatus;

    bool m_isConnected;

    void setupUI();
};

#endif // SETTINGSPAGE_H
