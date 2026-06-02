#include "mainwidget.h"
#include "overviewpage.h"
#include "trendpage.h"
#include "alarmpage.h"
#include "settingspage.h"

#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <QHeaderView>
#include <QIcon>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMqttTopicFilter>
#include <QMqttTopicName>
#include <QMqttSubscription>
#include <QUrl>
#include <QSslConfiguration>

mainWidget::mainWidget(QWidget *parent) :
    QWidget(parent),
    m_lastTemperature(0.0),
    m_lastHumidity(0.0),
    m_tempMaxLimit(40.0),
    m_humiMaxLimit(80.0),
    m_nextRequestId(0)
{
    setWindowTitle("边缘物联网网关监控终端");
    resize(1200, 800);
    setMinimumSize(1024, 680);
    setStyleSheet("background-color: #1E272E; color: #FFFFFF; font-family: 'Microsoft YaHei', sans-serif;");

    QIcon windowIcon(QString(":/new/prefix1/imgs/favicon.ico"));
    this->setWindowIcon(windowIcon);

    setupMqtt();
    setupTabs();
}

mainWidget::~mainWidget()
{
    if (m_mqttClient && m_mqttClient->state() == QMqttClient::Connected)
        m_mqttClient->disconnectFromHost();
}

// ============================================================
// embsky MQTT 初始化 (唯一客户端)
// ============================================================

void mainWidget::setupMqtt()
{
    m_mqttClient = new QMqttClient(this);
    m_mqttClient->setProtocolVersion(QMqttClient::MQTT_3_1_1);
    m_mqttClient->setCleanSession(true);

    connect(m_mqttClient, &QMqttClient::connected, this, &mainWidget::onMqttConnected);
    connect(m_mqttClient, &QMqttClient::disconnected, this, &mainWidget::onMqttDisconnected);
    connect(m_mqttClient, &QMqttClient::errorChanged, this,
            [this](QMqttClient::ClientError e) { onMqttError(e); });
}

// ============================================================
// Tab 页面
// ============================================================

void mainWidget::setupTabs()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #576574; background: #1E272E; }"
        "QTabBar::tab { background: #2F3542; color: #CCCCCC; padding: 12px 24px; "
        "    border: 1px solid #576574; border-bottom: none; margin-right: 2px; "
        "    font-size: 15px; font-weight: bold; }"
        "QTabBar::tab:selected { background: #00D2D3; color: #1E272E; }"
        "QTabBar::tab:hover:!selected { background: #57606F; }"
    );

    m_overviewPage = new OverviewPage(this);
    m_trendPage = new TrendPage(this);
    m_alarmPage = new AlarmPage(this);
    m_settingsPage = new SettingsPage(this);

    m_tabWidget->addTab(m_overviewPage, "概览");
    m_tabWidget->addTab(m_trendPage, "趋势");
    m_tabWidget->addTab(m_alarmPage, "报警");
    m_tabWidget->addTab(m_settingsPage, "设置");

    mainLayout->addWidget(m_tabWidget);

    // ---- embsky 云端信号 ----
    connect(m_settingsPage, &SettingsPage::embskyConnectRequested,
            this, &mainWidget::onEmbskyConnectRequested);
    connect(m_settingsPage, &SettingsPage::embskyDisconnectRequested,
            this, &mainWidget::onEmbskyDisconnectRequested);

    // ---- 业务信号 ----
    connect(m_alarmPage, &AlarmPage::thresholdsChanged,
            this, &mainWidget::onThresholdsChanged);
    connect(m_alarmPage, &AlarmPage::manualAlarmRequested,
            this, &mainWidget::onManualAlarmRequested);
    connect(m_alarmPage, &AlarmPage::clearHistoryRequested,
            this, &mainWidget::onClearHistoryRequested);
}

// ============================================================
// embsky 云端连接管理
// ============================================================

void mainWidget::onEmbskyConnectRequested(const EmbskyConfig &config)
{
    qDebug() << "onEmbskyConnectRequested:" << config.brokerHost
             << ":" << config.brokerPort << "username:" << config.username;

    m_embskyConfig = config;

    m_mqttClient->setHostname(config.brokerHost);
    m_mqttClient->setPort(config.brokerPort);
    m_mqttClient->setUsername(config.username);
    m_mqttClient->setPassword(config.password);
    m_mqttClient->setClientId(config.clientId);
    m_mqttClient->setKeepAlive(config.keepAlive);

    qDebug() << "Connecting to embsky:" << config.brokerHost << ":" << config.brokerPort;
    m_mqttClient->connectToHost();
}

void mainWidget::onEmbskyDisconnectRequested()
{
    qDebug() << "Disconnecting from embsky...";
    if (m_mqttClient && m_mqttClient->state() == QMqttClient::Connected) {
        m_mqttClient->disconnectFromHost();
    }
    m_settingsPage->updateEmbskyConnectionStatus(false);
}

void mainWidget::onMqttConnected()
{
    qDebug() << "embsky MQTT connected successfully";
    m_settingsPage->updateEmbskyConnectionStatus(true);
    m_overviewPage->updateConnectionStatus(true);
    m_trendPage->updateConnectionStatus(true);
    m_alarmPage->updateConnectionStatus(true);

    subscribeTopics();
    // 阈值通过 gateway/thresholds（Retained）自动获取
    requestAlarmLogs(0, QDateTime::currentSecsSinceEpoch(), 500);
}

void mainWidget::onMqttDisconnected()
{
    qDebug() << "embsky MQTT disconnected";
    m_settingsPage->updateEmbskyConnectionStatus(false);
    m_overviewPage->updateConnectionStatus(false);
    m_trendPage->updateConnectionStatus(false);
    m_alarmPage->updateConnectionStatus(false);
}

void mainWidget::onMqttError(QMqttClient::ClientError error)
{
    QString errMsg;
    switch (error) {
    case QMqttClient::NoError:                errMsg = "无错误"; break;
    case QMqttClient::InvalidProtocolVersion: errMsg = "协议版本不匹配"; break;
    case QMqttClient::IdRejected:             errMsg = "Client ID 被拒绝 (请更换唯一ID)"; break;
    case QMqttClient::ServerUnavailable:      errMsg = "embsky 服务器不可达 (检查地址和端口)"; break;
    case QMqttClient::BadUsernameOrPassword:  errMsg = "鉴权失败 (检查用户名和密码)"; break;
    case QMqttClient::NotAuthorized:          errMsg = "未授权访问"; break;
    case QMqttClient::TransportInvalid:       errMsg = "传输层错误 (检查 Broker 地址)"; break;
    case QMqttClient::ProtocolViolation:      errMsg = "协议违规 (MQTT 3.1.1 要求)"; break;
    case QMqttClient::UnknownError:           errMsg = "未知错误"; break;
    case QMqttClient::Mqtt5SpecificError:     errMsg = "MQTT 5.0 特定错误"; break;
    default:                                  errMsg = "embsky 错误: " + QString::number((int)error); break;
    }
    qDebug() << "embsky MQTT Error:" << errMsg << "code:" << (int)error;

    if (error != QMqttClient::NoError) {
        m_settingsPage->updateEmbskyConnectionStatus(false);
        m_overviewPage->updateConnectionStatus(false);
        m_trendPage->updateConnectionStatus(false);
        m_alarmPage->updateConnectionStatus(false);
    }
}

bool mainWidget::isMqttConnected() const
{
    return m_mqttClient && m_mqttClient->state() == QMqttClient::Connected;
}

void mainWidget::updateConnectionStatus(bool connected)
{
    if (connected) {
        // 由 onMqttConnected 处理
    } else {
        onMqttDisconnected();
    }
}

// ============================================================
// Topic 订阅
// ============================================================

void mainWidget::subscribeTopics()
{
    // 温度传感器 — eslink/5800/5910/16790
    if (!m_embskyConfig.tempTopic.isEmpty()) {
        auto *subTemp = m_mqttClient->subscribe(QMqttTopicFilter(m_embskyConfig.tempTopic), 0);
        if (subTemp) {
            connect(subTemp, &QMqttSubscription::messageReceived, this,
                    [this](QMqttMessage msg) {
                        onSensorData(msg.payload(), true);  // true = temperature
                    });
        }
        qDebug() << "Subscribed to temp topic:" << m_embskyConfig.tempTopic;
    }

    // 湿度传感器 — eslink/5800/5910/16789
    if (!m_embskyConfig.humiTopic.isEmpty()) {
        auto *subHumi = m_mqttClient->subscribe(QMqttTopicFilter(m_embskyConfig.humiTopic), 0);
        if (subHumi) {
            connect(subHumi, &QMqttSubscription::messageReceived, this,
                    [this](QMqttMessage msg) {
                        onSensorData(msg.payload(), false); // false = humidity
                    });
        }
        qDebug() << "Subscribed to humi topic:" << m_embskyConfig.humiTopic;
    }

    // gateway/alarm — 报警通知（保留）
    auto *subAlarm = m_mqttClient->subscribe(QMqttTopicFilter("gateway/alarm"), 1);
    if (subAlarm) {
        connect(subAlarm, &QMqttSubscription::messageReceived, this,
                [this](QMqttMessage msg) { onAlarmMessage(msg.payload()); });
    }

    // gateway/thresholds — 阈值（Retained）
    auto *subThresh = m_mqttClient->subscribe(QMqttTopicFilter("gateway/thresholds"), 1);
    if (subThresh) {
        connect(subThresh, &QMqttSubscription::messageReceived, this,
                [this](QMqttMessage msg) { onThresholdsMessage(msg.payload()); });
    }
}

// ============================================================
// MQTT 消息处理 — eslink 传感器数据
// ============================================================

void mainWidget::onSensorData(const QByteArray &payload, bool isTemperature)
{
    // eslink 传感器数据格式: {"data": 25.3, "timestamp": 1717250000}
    // 也可能只有 value 字段
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    double value = 0.0;
    qint64 ts = QDateTime::currentSecsSinceEpoch();

    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        value = obj["data"].toDouble(obj["value"].toDouble(0.0));
        if (obj.contains("timestamp"))
            ts = (qint64)obj["timestamp"].toDouble();
    } else {
        // Try plain number
        bool ok;
        value = payload.toDouble(&ok);
        if (!ok) return;
    }

    qDebug() << (isTemperature ? "Temp" : "Humi") << "received:" << value
             << "at" << QDateTime::fromSecsSinceEpoch(ts).toString("hh:mm:ss");

    if (isTemperature) {
        m_lastTemperature = value;
    } else {
        m_lastHumidity = value;
    }

    // Submit immediately — 不等待另一个传感器，用缓存值补全
    DeviceDataPacket packet;
    packet.dev_id = 1;
    packet.temperature = m_lastTemperature;
    packet.humidity = m_lastHumidity;
    packet.timestamp = ts;
    processReceivedData(packet);
}

void mainWidget::onAlarmMessage(const QByteArray &payload)
{
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();

    AlarmNotificationRecord notif;
    memset(&notif, 0, sizeof(notif));
    notif.alarm_id     = obj["alarm_id"].toInt();
    notif.dev_id       = obj["dev_id"].toInt(1);
    notif.limit_value  = obj["limit_value"].toDouble();
    notif.actual_value = obj["actual_value"].toDouble();
    notif.status       = obj["status"].toInt();
    notif.occur_time   = (int64_t)obj["occur_time"].toDouble();

    QByteArray typeBytes = obj["alarm_type"].toString().toUtf8();
    strncpy(notif.alarm_type, typeBytes.constData(), sizeof(notif.alarm_type) - 1);
    notif.alarm_type[sizeof(notif.alarm_type) - 1] = '\0';

    m_alarmPage->onAlarmNotificationReceived(notif);
}

void mainWidget::onThresholdsMessage(const QByteArray &payload)
{
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();

    m_tempMaxLimit = obj["temp_max"].toDouble(40.0);
    m_humiMaxLimit = obj["humi_max"].toDouble(80.0);

    m_overviewPage->updateThresholds(m_tempMaxLimit, m_humiMaxLimit);
    m_alarmPage->updateThresholds(m_tempMaxLimit, m_humiMaxLimit);
}

// ============================================================
// 实时数据处理 (纯接收，不转发)
// ============================================================

void mainWidget::processReceivedData(const DeviceDataPacket &packet)
{
    // 更新 UI 显示
    m_overviewPage->updateSensorData(packet);
    m_trendPage->updateSensorData(packet);
    m_alarmPage->updateSensorData(packet);

    qDebug() << "Data received from embsky: dev_id=" << packet.dev_id
             << "temp=" << packet.temperature << "humi=" << packet.humidity;
}

// ============================================================
// 客户端 → 服务器 请求（MQTT Publish JSON）
// ============================================================

void mainWidget::requestHistory(int devId, const QString &varName,
                                 qint64 startTime, qint64 endTime, int limit)
{
    if (!isMqttConnected()) return;

    QJsonObject req;
    req["dev_id"]     = devId;
    req["var_name"]   = varName;
    req["start_time"] = (double)startTime;
    req["end_time"]   = (double)endTime;
    req["limit"]      = limit;
    req["request_id"] = m_nextRequestId++;

    QByteArray json = QJsonDocument(req).toJson(QJsonDocument::Compact);
    m_mqttClient->publish(QMqttTopicName("gateway/history/req"), json, 1);
}

void mainWidget::requestAlarmLogs(qint64 startTime, qint64 endTime, int limit)
{
    if (!isMqttConnected()) return;

    QJsonObject req;
    req["start_time"] = (double)startTime;
    req["end_time"]   = (double)endTime;
    req["limit"]      = limit;
    req["request_id"] = m_nextRequestId++;

    QByteArray json = QJsonDocument(req).toJson(QJsonDocument::Compact);
    m_mqttClient->publish(QMqttTopicName("gateway/alarm/req"), json, 1);
}

void mainWidget::sendThresholdConfig(double tempMax, double humiMax)
{
    if (!isMqttConnected()) return;

    QJsonObject req;
    req["temp_max"] = tempMax;
    req["humi_max"] = humiMax;

    QByteArray json = QJsonDocument(req).toJson(QJsonDocument::Compact);
    m_mqttClient->publish(QMqttTopicName("gateway/thresholds/set"), json, 1);
}

// ============================================================
// 事件处理
// ============================================================

void mainWidget::onThresholdsChanged(double tempMax, double humiMax)
{
    m_tempMaxLimit = tempMax;
    m_humiMaxLimit = humiMax;
    m_overviewPage->updateThresholds(tempMax, humiMax);
    m_alarmPage->updateThresholds(tempMax, humiMax);
    // 同步到 embsky MQTT 服务器
    sendThresholdConfig(tempMax, humiMax);
}

void mainWidget::onManualAlarmRequested()
{
    if (!isMqttConnected()) {
        QMessageBox::information(this, "提示", "请先连接 embsky Broker。");
        return;
    }
    DeviceDataPacket p;
    p.dev_id = 1;
    p.temperature = m_tempMaxLimit + 5.0;
    p.humidity = 50.0;
    p.timestamp = QDateTime::currentSecsSinceEpoch();
    processReceivedData(p);
}

void mainWidget::onClearHistoryRequested()
{
    // Handled internally by AlarmPage
}
