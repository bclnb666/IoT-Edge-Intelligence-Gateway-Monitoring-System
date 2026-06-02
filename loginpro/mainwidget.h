#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QMqttClient>
#include <QTabWidget>
#include <QVector>
#include "protocol.h"

class OverviewPage;
class TrendPage;
class AlarmPage;
class SettingsPage;

class mainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit mainWidget(QWidget *parent = nullptr);
    ~mainWidget() override;

    // 供子页面调用的查询/设置方法
    void requestHistory(int devId, const QString &varName,
                        qint64 startTime, qint64 endTime, int limit = 2000);
    void requestAlarmLogs(qint64 startTime, qint64 endTime, int limit = 500);
    void sendThresholdConfig(double tempMax, double humiMax);

    QMqttClient* mqttClient() const { return m_mqttClient; }
    bool isMqttConnected() const;

public slots:
    void updateConnectionStatus(bool connected);

private slots:
    // ---- embsky MQTT 连接管理 ----
    void onEmbskyConnectRequested(const EmbskyConfig &config);
    void onEmbskyDisconnectRequested();
    void onMqttConnected();
    void onMqttDisconnected();
    void onMqttError(QMqttClient::ClientError error);

    // ---- 业务事件 ----
    void onThresholdsChanged(double tempMax, double humiMax);
    void onManualAlarmRequested();
    void onClearHistoryRequested();

    // ---- MQTT 消息处理 ----
    void onSensorData(const QByteArray &payload, bool isTemperature);
    void onAlarmMessage(const QByteArray &payload);
    void onThresholdsMessage(const QByteArray &payload);

private:
    QTabWidget *m_tabWidget;

    OverviewPage *m_overviewPage;
    TrendPage *m_trendPage;
    AlarmPage *m_alarmPage;
    SettingsPage *m_settingsPage;

    // embsky 云端 MQTT 客户端 (唯一)
    QMqttClient *m_mqttClient;
    EmbskyConfig m_embskyConfig;

    // 传感器数据缓存（任一传感器到达即提交，用缓存补全另一值）
    double m_lastTemperature;
    double m_lastHumidity;

    double m_tempMaxLimit;
    double m_humiMaxLimit;

    int m_nextRequestId;

    void setupTabs();
    void setupMqtt();
    void processReceivedData(const DeviceDataPacket &packet);
    void subscribeTopics();
};

#endif // MAINWIDGET_H
