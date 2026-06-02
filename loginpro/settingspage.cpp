#include "settingspage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QDateTime>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent),
      m_isConnected(false)
{
    setupUI();
}

void SettingsPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    QString groupStyle =
        "QGroupBox { font-weight: bold; font-size: 14px; color: #00D2D3; "
        "border: 1px solid #576574; border-radius: 8px; margin-top: 15px; padding: 14px; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }";

    QString lineEditStyle =
        "background-color: #2F3542; color: white; padding: 8px; border-radius: 4px; font-size: 14px;";

    // ============================================================
    // embsky 云平台配置 GroupBox
    // ============================================================
    QGroupBox *boxEmbsky = new QGroupBox("embsky 云平台配置", this);
    boxEmbsky->setStyleSheet(groupStyle);
    QFormLayout *embskyForm = new QFormLayout(boxEmbsky);
    embskyForm->setSpacing(12);

    m_txtEmbskyHost = new QLineEdit("iot.embsky.com", this);
    m_txtEmbskyHost->setStyleSheet(lineEditStyle);
    embskyForm->addRow(new QLabel("Broker 地址:"), m_txtEmbskyHost);

    m_spinEmbskyPort = new QSpinBox(this);
    m_spinEmbskyPort->setRange(1, 65535);
    m_spinEmbskyPort->setValue(1883);
    m_spinEmbskyPort->setStyleSheet(
        "QSpinBox { background-color: #2F3542; color: white; padding: 8px; "
        "border-radius: 4px; font-size: 14px; }");
    embskyForm->addRow(new QLabel("MQTT 端口:"), m_spinEmbskyPort);

    m_txtEmbskyUsername = new QLineEdit(this);
    m_txtEmbskyUsername->setPlaceholderText("MQTT 用户名");
    m_txtEmbskyUsername->setStyleSheet(lineEditStyle);
    embskyForm->addRow(new QLabel("用户名:"), m_txtEmbskyUsername);

    m_txtEmbskyPassword = new QLineEdit(this);
    m_txtEmbskyPassword->setPlaceholderText("MQTT 密码");
    m_txtEmbskyPassword->setEchoMode(QLineEdit::Password);
    m_txtEmbskyPassword->setStyleSheet(lineEditStyle);
    embskyForm->addRow(new QLabel("密码:"), m_txtEmbskyPassword);

    m_txtEmbskyClientId = new QLineEdit(this);
    m_txtEmbskyClientId->setPlaceholderText("客户端ID (必须唯一，留空自动生成)");
    m_txtEmbskyClientId->setStyleSheet(lineEditStyle);
    embskyForm->addRow(new QLabel("Client ID:"), m_txtEmbskyClientId);

    // ---- 传感器 Topic 配置 ----
    QLabel *lblTopicInfo = new QLabel("传感器 Topic（云平台 → 网关发布数据的主题）", this);
    lblTopicInfo->setStyleSheet("color: #00D2D3; font-size: 13px; font-weight: bold; padding: 4px 0;");
    embskyForm->addRow(new QLabel(""), lblTopicInfo);

    m_txtTempTopic = new QLineEdit("eslink/5800/5910/16790", this);
    m_txtTempTopic->setPlaceholderText("温度传感器 Topic");
    m_txtTempTopic->setStyleSheet(lineEditStyle);
    embskyForm->addRow(new QLabel("温度 Topic:"), m_txtTempTopic);

    m_txtHumiTopic = new QLineEdit("eslink/5800/5910/16789", this);
    m_txtHumiTopic->setPlaceholderText("湿度传感器 Topic");
    m_txtHumiTopic->setStyleSheet(lineEditStyle);
    embskyForm->addRow(new QLabel("湿度 Topic:"), m_txtHumiTopic);

    mainLayout->addWidget(boxEmbsky);

    // ============================================================
    // 连接按钮
    // ============================================================
    m_btnConnect = new QPushButton("连接 Broker", this);
    m_btnConnect->setStyleSheet(
        "QPushButton { background-color: #2ECC71; color: white; font-weight: bold; "
        "border-radius: 5px; padding: 12px; font-size: 15px; } "
        "QPushButton:hover { background-color: #27AE60; } "
        "QPushButton:disabled { background-color: #57606F; }");
    connect(m_btnConnect, &QPushButton::clicked, this, &SettingsPage::onConnectClicked);
    mainLayout->addWidget(m_btnConnect);

    // ============================================================
    // 连接状态
    // ============================================================
    m_lblConnStatus = new QLabel("● embsky: 未连接", this);
    m_lblConnStatus->setStyleSheet("font-size: 14px; font-weight: bold; color: #E74C3C; padding: 6px;");
    mainLayout->addWidget(m_lblConnStatus);

    // ============================================================
    // 系统信息 GroupBox
    // ============================================================
    QGroupBox *boxInfo = new QGroupBox("系统信息", this);
    boxInfo->setStyleSheet(groupStyle);
    QVBoxLayout *infoLayout = new QVBoxLayout(boxInfo);

    auto addInfoRow = [&](const QString &label, const QString &value) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *lbl = new QLabel(label, this);
        lbl->setStyleSheet("font-weight: normal; color: #AAAAAA;");
        QLabel *val = new QLabel(value, this);
        val->setStyleSheet("font-weight: bold; color: #FFFFFF;");
        row->addWidget(lbl);
        row->addStretch();
        row->addWidget(val);
        infoLayout->addLayout(row);
    };

    addInfoRow("应用版本:", "v1.0.0");
    addInfoRow("Qt 版本:", QT_VERSION_STR);
    addInfoRow("协议类型:", "MQTT 3.1.1 (embsky 云平台)");
    addInfoRow("数据来源:", "STM32 → 网关 → HTTP → embsky → MQTT");

    mainLayout->addWidget(boxInfo);
    mainLayout->addStretch();
}

// ============================================================
// 连接按钮
// ============================================================

void SettingsPage::onConnectClicked()
{
    if (m_isConnected) {
        // 断开连接
        m_isConnected = false;
        emit embskyDisconnectRequested();
        return;
    }

    // 连接 - 校验必填字段
    EmbskyConfig config = embskyConfig();
    if (config.brokerHost.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入 Broker 地址。");
        return;
    }
    if (config.username.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户名。");
        return;
    }
    if (config.password.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入密码。");
        return;
    }
    if (config.clientId.isEmpty()) {
        // 自动生成 client ID
        config.clientId = "qt_gateway_" + QString::number(QDateTime::currentSecsSinceEpoch());
        m_txtEmbskyClientId->setText(config.clientId);
    }
    emit embskyConnectRequested(config);
}

// ============================================================
// 连接状态更新
// ============================================================

void SettingsPage::updateEmbskyConnectionStatus(bool connected)
{
    m_isConnected = connected;
    if (connected) {
        m_lblConnStatus->setText("● embsky: 已连接");
        m_lblConnStatus->setStyleSheet("font-size: 14px; font-weight: bold; color: #2ECC71; padding: 6px;");
        m_btnConnect->setText("断开连接");
        m_btnConnect->setStyleSheet(
            "QPushButton { background-color: #E74C3C; color: white; font-weight: bold; "
            "border-radius: 5px; padding: 12px; font-size: 15px; } "
            "QPushButton:hover { background-color: #C0392B; }");
        // 连接后禁用配置修改
        m_txtEmbskyHost->setEnabled(false);
        m_spinEmbskyPort->setEnabled(false);
        m_txtEmbskyUsername->setEnabled(false);
        m_txtEmbskyPassword->setEnabled(false);
        m_txtEmbskyClientId->setEnabled(false);
        m_txtTempTopic->setEnabled(false);
        m_txtHumiTopic->setEnabled(false);
    } else {
        m_lblConnStatus->setText("● embsky: 未连接");
        m_lblConnStatus->setStyleSheet("font-size: 14px; font-weight: bold; color: #E74C3C; padding: 6px;");
        m_btnConnect->setText("连接 Broker");
        m_btnConnect->setStyleSheet(
            "QPushButton { background-color: #2ECC71; color: white; font-weight: bold; "
            "border-radius: 5px; padding: 12px; font-size: 15px; } "
            "QPushButton:hover { background-color: #27AE60; }");
        // 恢复配置修改
        m_txtEmbskyHost->setEnabled(true);
        m_spinEmbskyPort->setEnabled(true);
        m_txtEmbskyUsername->setEnabled(true);
        m_txtEmbskyPassword->setEnabled(true);
        m_txtEmbskyClientId->setEnabled(true);
        m_txtTempTopic->setEnabled(true);
        m_txtHumiTopic->setEnabled(true);
    }
}

// ============================================================
// 访问器
// ============================================================

EmbskyConfig SettingsPage::embskyConfig() const
{
    EmbskyConfig cfg;
    cfg.brokerHost = m_txtEmbskyHost->text().trimmed();
    cfg.brokerPort = static_cast<quint16>(m_spinEmbskyPort->value());
    cfg.username   = m_txtEmbskyUsername->text().trimmed();
    cfg.password   = m_txtEmbskyPassword->text();
    cfg.clientId   = m_txtEmbskyClientId->text().trimmed();
    cfg.keepAlive  = 60;
    cfg.tempTopic  = m_txtTempTopic->text().trimmed();
    cfg.humiTopic  = m_txtHumiTopic->text().trimmed();
    return cfg;
}
