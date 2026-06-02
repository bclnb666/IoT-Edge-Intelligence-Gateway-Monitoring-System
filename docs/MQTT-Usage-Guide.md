# 📡 MQTT 使用详解 / MQTT Usage Guide

> IoT Edge Intelligence Gateway — embsky 云平台 MQTT 通信完整指南

---

## 目录 / Table of Contents

1. [架构概述](#1-架构概述)
2. [MQTT 客户端初始化](#2-mqtt-客户端初始化)
3. [Broker 连接配置](#3-broker-连接配置)
4. [主题设计 / Topic Design](#4-主题设计)
5. [消息格式 / Message Format](#5-消息格式)
6. [数据流详解](#6-数据流详解)
7. [SQLite 本地存储](#7-sqlite-本地存储)
8. [告警机制](#8-告警机制)
9. [连接生命周期](#9-连接生命周期)
10. [错误处理](#10-错误处理)
11. [最佳实践](#11-最佳实践)
12. [故障排查](#12-故障排查)

---

## 1. 架构概述

```
                        MQTT 通信拓扑 / Topology

   ┌──────────────┐                              ┌──────────────────────┐
   │  STM32 设备   │                              │   ☁️ embsky Cloud     │
   │  传感器采集   │── UDP Multicast ──► Gateway ──│   MQTT Broker         │
   └──────────────┘              224.1.2.3:8081  │   iot.embsky.com:1883│
                                                  └────┬────────┬────────┘
                                                       │        │
                                              Subscribe │        │ Publish
                                                        │        │
                                                  ┌─────▼────────▼──────┐
                                                  │  🖥️ Qt6 Desktop App  │
                                                  │  QMqttClient (单例)  │
                                                  │  MQTT 3.1.1 / TCP   │
                                                  └──────────────────────┘
```

| 角色 | 组件 | 协议 |
|:---:|------|:---:|
| **Publisher** | embsky Cloud (数据接收端) | MQTT 3.1.1 |
| **Broker** | `iot.embsky.com:1883` | MQTT Broker |
| **Subscriber** | Qt6 Desktop Client | QMqttClient |
| **传输层** | 原生 TCP (未启用 TLS/WebSocket) | TCP :1883 |

---

## 2. MQTT 客户端初始化

### 2.1 创建客户端

**文件:** `loginpro/mainwidget.cpp` → `setupMqtt()`

```cpp
#include <QMqttClient>

// 创建单例 MQTT 客户端
m_mqttClient = new QMqttClient(this);

// 固定使用 MQTT 3.1.1 协议
m_mqttClient->setProtocolVersion(QMqttClient::MQTT_3_1_1);

// Clean Session = true (断开后不保留会话状态)
m_mqttClient->setCleanSession(true);
```

| 参数 | 值 | 说明 |
|------|:---:|------|
| `protocolVersion` | `MQTT_3_1_1` | 固定不可选，兼容 embsky |
| `cleanSession` | `true` | 每次连接从零开始，不保留订阅和未完成消息 |
| `clientId` | `"qt_gateway_" + 时间戳` | 自动生成，保证唯一性 |
| `keepAlive` | `60` 秒 | 心跳间隔 |

### 2.2 信号连接

```cpp
// 连接成功
connect(m_mqttClient, &QMqttClient::connected,
        this, &mainWidget::onMqttConnected);

// 断开连接
connect(m_mqttClient, &QMqttClient::disconnected,
        this, &mainWidget::onMqttDisconnected);

// 错误处理
connect(m_mqttClient, &QMqttClient::errorChanged,
        this, [this](QMqttClient::ClientError error) {
            onMqttError(error);
        });
```

> ⚠️ **注意:** 本项目未监听 `stateChanged` 和 `pingResponseReceived` 信号，但生产环境建议监听以监控连接健康度。

---

## 3. Broker 连接配置

### 3.1 配置数据结构

**文件:** `loginpro/protocol.h`

```cpp
struct EmbskyConfig {
    QString brokerHost;   // "iot.embsky.com"
    quint16 brokerPort;   // 1883
    QString username;     // embsky 账号
    QString password;     // embsky 密码
    QString clientId;     // 为空则自动生成
    quint16 keepAlive;    // 60 秒
    QString tempTopic;    // "eslink/5800/5910/16790"
    QString humiTopic;    // "eslink/5800/5910/16789"
};
```

### 3.2 连接流程 / Connection Flow

```
  SettingsPage UI                mainWidget              QMqttClient          embsky Broker
  ──────────────                ──────────              ────────────          ─────────────
       │                            │                        │                     │
       │  用户点击 "Connect Broker"  │                        │                     │
       │──验证表单字段──────────────>│                        │                     │
       │  (host/user/pass 非空)      │                        │                     │
       │                            │                        │                     │
       │  emit embskyConnectRequested(config)                 │                     │
       │──────────────────────────>│                        │                     │
       │                            │ setHostname()          │                     │
       │                            │ setPort()              │                     │
       │                            │ setUsername()          │                     │
       │                            │ setPassword()          │                     │
       │                            │ setClientId()          │                     │
       │                            │ setKeepAlive()         │                     │
       │                            │ connectToHost() ──────>│                     │
       │                            │                        │── TCP CONNECT ────>│
       │                            │                        │<── CONNACK ───────│
       │                            │<── connected signal ───│                     │
       │                            │ subscribeTopics()      │                     │
       │                            │ (订阅 4 个主题)         │                     │
       │                            │ requestAlarmLogs()     │                     │
       │<── statusUpdated ──────────│                        │                     │
       │  "Connected" (绿色)        │                        │                     │
```

### 3.3 连接代码 / Connect Code

```cpp
void mainWidget::onEmbskyConnectRequested(const EmbskyConfig &config)
{
    m_embskyConfig = config;

    m_mqttClient->setHostname(config.brokerHost);     // "iot.embsky.com"
    m_mqttClient->setPort(config.brokerPort);          // 1883
    m_mqttClient->setUsername(config.username);
    m_mqttClient->setPassword(config.password);
    m_mqttClient->setClientId(clientId.isEmpty()
        ? QString("qt_gateway_%1").arg(QDateTime::currentSecsSinceEpoch())
        : clientId);
    m_mqttClient->setKeepAlive(60);

    m_mqttClient->connectToHost();  // 发起 TCP 连接
}
```

### 3.4 断开连接 / Disconnect

```cpp
void mainWidget::onEmbskyDisconnectRequested()
{
    m_mqttClient->disconnectFromHost();
    // 触发 onMqttDisconnected() → 所有页面标记未连接
}
```

---

## 4. 主题设计

### 4.1 主题总览 / Topic Map

```
                         Qt6 Desktop Client
                              │
        ┌─────────────────────┼─────────────────────┐
        │  Subscribe          │  Publish            │
        │  (4 topics)         │  (3 topics)         │
        │                     │                     │
        ▼                     ▼                     ▼
   ┌──────────────┐    ┌──────────────────┐
   │ Sensor Data  │    │ Control Commands │
   │ QoS 0        │    │ QoS 1            │
   └──────────────┘    └──────────────────┘
```

### 4.2 订阅主题 / Subscribe Topics

| # | 主题 | QoS | Retained | 触发时机 | 用途 |
|:--:|------|:---:|:--------:|----------|------|
| 1 | `{tempTopic}` | **0** | ❌ | 每次传感器上报 | 温度实时数据 |
| 2 | `{humiTopic}` | **0** | ❌ | 每次传感器上报 | 湿度实时数据 |
| 3 | `gateway/alarm` | **1** | ✅ | 告警触发时 | 服务端告警通知 |
| 4 | `gateway/thresholds` | **1** | ✅ | 阈值变更时 | 阈值配置同步 |

**默认温度主题:** `eslink/5800/5910/16790`
**默认湿度主题:** `eslink/5800/5910/16789`

> **QoS 策略说明:**
> - **QoS 0** (至多一次) — 传感器数据高频发送，允许个别丢包
> - **QoS 1** (至少一次) — 告警和阈值必须送达，不可丢失
> - **Retained** — broker 保留最后一条消息，新订阅者立即收到

### 4.3 发布主题 / Publish Topics

| # | 主题 | QoS | 触发时机 | 用途 |
|:--:|------|:---:|----------|------|
| 1 | `gateway/history/req` | **1** | 手动请求或初始化 | 请求历史传感器数据 |
| 2 | `gateway/alarm/req` | **1** | 连接成功后自动 | 请求历史告警记录 |
| 3 | `gateway/thresholds/set` | **1** | 用户修改阈值 | 下发新阈值配置 |

### 4.4 订阅实现代码 / Subscribe Implementation

```cpp
void mainWidget::subscribeTopics()
{
    // 1. 订阅温度传感器 (QoS 0)
    auto *tempSub = m_mqttClient->subscribe(
        QMqttTopicFilter(m_embskyConfig.tempTopic), 0);
    connect(tempSub, &QMqttSubscription::messageReceived,
            this, [this](QMqttMessage msg) {
                onSensorData(msg.payload(), true);  // true = 温度
            });

    // 2. 订阅湿度传感器 (QoS 0)
    auto *humiSub = m_mqttClient->subscribe(
        QMqttTopicFilter(m_embskyConfig.humiTopic), 0);
    connect(humiSub, &QMqttSubscription::messageReceived,
            this, [this](QMqttMessage msg) {
                onSensorData(msg.payload(), false); // false = 湿度
            });

    // 3. 订阅告警通知 (QoS 1, Retained)
    auto *alarmSub = m_mqttClient->subscribe(
        QMqttTopicFilter("gateway/alarm"), 1);
    connect(alarmSub, &QMqttSubscription::messageReceived,
            this, [this](QMqttMessage msg) {
                onAlarmMessage(msg.payload());
            });

    // 4. 订阅阈值配置 (QoS 1, Retained)
    auto *threshSub = m_mqttClient->subscribe(
        QMqttTopicFilter("gateway/thresholds"), 1);
    connect(threshSub, &QMqttSubscription::messageReceived,
            this, [this](QMqttMessage msg) {
                onThresholdsMessage(msg.payload());
            });
}
```

---

## 5. 消息格式

### 5.1 传感器数据 / Sensor Data (入站)

**主题:** `eslink/5800/5910/16790` (温度) / `16789` (湿度)
**方向:** Cloud → UI
**QoS:** 0

```json
{
    "data": 25.3,
    "timestamp": 1717250000
}
```

**备用格式 (兼容):**

```json
{
    "value": 60.5
}
```

**纯数字格式 (兼容):**

```
25.3
```

**解析逻辑:**

```cpp
void mainWidget::onSensorData(const QByteArray &payload, bool isTemp)
{
    QJsonDocument doc = QJsonDocument::fromJson(payload);

    double value = 0.0;
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        // 优先取 "data" 字段，其次 "value" 字段
        value = obj["data"].toDouble(obj["value"].toDouble(0.0));
    } else {
        // 纯数字格式兼容
        bool ok;
        value = payload.toDouble(&ok);
        if (!ok) return;  // 解析失败，静默忽略
    }

    // 缓存值
    if (isTemp)
        m_lastTemperature = value;
    else
        m_lastHumidity = value;

    // 构造完整数据包 (包含两种传感器的最新值)
    DeviceDataPacket packet;
    packet.dev_id = 1;
    packet.temperature = m_lastTemperature;
    packet.humidity = m_lastHumidity;
    packet.timestamp = doc.isObject()
        ? (qint64)obj["timestamp"].toDouble()
        : QDateTime::currentSecsSinceEpoch();

    processReceivedData(packet);  // 分发给所有 UI 页面
}
```

### 5.2 告警通知 / Alarm Notification (入站)

**主题:** `gateway/alarm`
**方向:** Cloud → UI
**QoS:** 1, Retained

```json
{
    "alarm_id": 42,
    "dev_id": 1,
    "alarm_type": "TemperatureHigh",
    "limit_value": 40.0,
    "actual_value": 55.3,
    "status": 1,
    "occur_time": 1717250000
}
```

| 字段 | 类型 | 说明 |
|------|:---:|------|
| `alarm_id` | int | 告警唯一标识 |
| `dev_id` | int | 设备 ID |
| `alarm_type` | string(32) | 告警类型，如 `"TemperatureHigh"` |
| `limit_value` | double | 触发告警的阈值 |
| `actual_value` | double | 实际测量值 |
| `status` | int | 告警状态 (1=触发, 0=恢复) |
| `occur_time` | int64 | 告警时间 (Unix 秒) |

### 5.3 阈值配置 / Threshold Config (入站)

**主题:** `gateway/thresholds`
**方向:** Cloud → UI
**QoS:** 1, Retained

```json
{
    "temp_max": 40.0,
    "humi_max": 80.0
}
```

### 5.4 历史数据请求 / History Request (出站)

**主题:** `gateway/history/req`
**方向:** UI → Cloud
**QoS:** 1

```json
{
    "dev_id": 1,
    "var_name": "temperature",
    "start_time": 1700000000,
    "end_time": 1717250000,
    "limit": 2000,
    "request_id": 1
}
```

| 字段 | 说明 |
|------|------|
| `dev_id` | 设备 ID (固定 1) |
| `var_name` | 变量名: `"temperature"` 或 `"humidity"` |
| `start_time` | 起始时间 (Unix 秒) |
| `end_time` | 结束时间 (Unix 秒) |
| `limit` | 最大返回条数 |
| `request_id` | 请求序号 (自增，用于匹配响应) |

### 5.5 告警记录请求 / Alarm Log Request (出站)

**主题:** `gateway/alarm/req`
**方向:** UI → Cloud
**QoS:** 1

```json
{
    "start_time": 0,
    "end_time": 1717250000,
    "limit": 500,
    "request_id": 2
}
```

### 5.6 阈值下发 / Threshold Set (出站)

**主题:** `gateway/thresholds/set`
**方向:** UI → Cloud → Gateway
**QoS:** 1

```json
{
    "temp_max": 40.0,
    "humi_max": 80.0
}
```

---

## 6. 数据流详解

### 6.1 完整的 MQTT 消息 → UI 更新路径

```
   embsky MQTT Broker
        │
        │  PUBLISH eslink/5800/5910/16790
        │  Payload: {"data":25.3,"timestamp":1717250000}
        ▼
   QMqttSubscription::messageReceived(msg)
        │
        ▼
   onSensorData(payload, isTemp=true)
        │
        ├─► JSON 解析 → value = 25.3
        │
        ├─► 缓存: m_lastTemperature = 25.3
        │
        ├─► 构造 DeviceDataPacket {temp=25.3, humi=m_lastHumidity}
        │
        └─► processReceivedData(packet)
                │
                ├─► OverviewPage::updateSensorData()
                │   ├─ 设置目标值: m_targetTemp = 25.3
                │   ├─ 判断告警: m_tempAlarm = (25.3 > m_tempMaxLimit)
                │   └─ 启动动画定时器 (30ms 间隔)
                │       └─ 线性插值: m_currentTemp += (target - current) * 0.08
                │           └─ update() → paintEvent()
                │               └─ 绘制仪表盘 (弧线 + 指针 + 刻度)
                │
                ├─► TrendPage::updateSensorData()
                │   ├─ storeDataPoint() → SQLite INSERT
                │   ├─ 追加到环形缓冲 (300 点上限)
                │   ├─ 丢弃 60 秒窗口外的旧数据
                │   ├─ flushSeries() → 原子替换 QSplineSeries 数据
                │   ├─ 更新浮动数值标签
                │   └─ 每 10 个点刷新历史表格 (SQLite SELECT)
                │
                └─► AlarmPage::updateSensorData()
                    ├─ 插入历史表格 (温度 + 湿度 各一行)
                    ├─ 检查: temp > tempMaxLimit?
                    │   └─ YES → triggerAlarm("温度过高")
                    ├─ 检查: humi > humiMaxLimit?
                    │   └─ YES → triggerAlarm("湿度过高")
                    └─ 更新状态指示器
```

### 6.2 双传感器缓存机制

```
  温度消息到达                      湿度消息到达
  ────────────                      ────────────
  value = 25.3                      value = 60.5
  m_lastTemperature = 25.3          m_lastHumidity = 60.5
  packet = {                        packet = {
      temp: 25.3   ← 新值              temp: m_lastTemperature  ← 缓存值
      humi: m_lastHumidity ← 缓存      humi: 60.5   ← 新值
  }                                  }
  processReceivedData(packet)        processReceivedData(packet)
```

> 💡 **设计意图:** 温度和湿度分成两个独立主题发布，但 UI 需要配对展示。无论哪个先到，
> 都能用另一个传感器的最新缓存值组装完整的数据包，保证每次 UI 更新都有两个值。

### 6.3 发布流程

```
  用户操作                     mainWidget                  QMqttClient
  ────────                    ──────────                  ────────────
      │                           │                            │
  AlarmPage                      │                            │
  spinBox valueChanged            │                            │
      │                           │                            │
      │──emit──────────────────>│                            │
      │  thresholdsChanged        │                            │
      │  (tempMax, humiMax)       │                            │
      │                           │                            │
      │                  检查 isMqttConnected()               │
      │                      ├─ NO → return; (静默)            │
      │                      └─ YES ↓                         │
      │                           │                            │
      │                  构造 JSON:                            │
      │                  {"temp_max":40,"humi_max":80}         │
      │                           │                            │
      │                  publish(QMqttTopicName,               │
      │                          QJsonDocument,                │
      │                          QoS=1) ──────────────────────>│
      │                                        PUBLISH ───────>│ Broker
```

---

## 7. SQLite 本地存储

### 7.1 数据库文件

**文件路径:** `trend_data.db` (应用程序工作目录)
**连接名:** `"trend_history"`

### 7.2 表结构

```sql
CREATE TABLE IF NOT EXISTS sensor_log (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp   INTEGER NOT NULL,
    temperature REAL,
    humidity    REAL
);

CREATE INDEX IF NOT EXISTS idx_ts ON sensor_log(timestamp);
```

### 7.3 写入策略

```cpp
void TrendPage::storeDataPoint(qint64 ts, double temp, double humi)
{
    QSqlDatabase db = QSqlDatabase::database("trend_history");
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO sensor_log (timestamp, temperature, humidity) "
        "VALUES (?, ?, ?)"
    );
    query.addBindValue(ts);
    query.addBindValue(temp);
    query.addBindValue(humi);
    query.exec();

    // 超过 DB_MAX (1000) 条时，删除最旧的
    query.exec("SELECT COUNT(*) FROM sensor_log");
    if (query.next()) {
        int count = query.value(0).toInt();
        int excess = count - DB_MAX;
        if (excess > 0) {
            query.prepare(
                "DELETE FROM sensor_log WHERE id IN "
                "(SELECT id FROM sensor_log ORDER BY id ASC LIMIT ?)"
            );
            query.addBindValue(excess);
            query.exec();
        }
    }
}
```

| 参数 | 值 | 说明 |
|------|:--:|------|
| `DB_MAX` | 1000 | 数据库最大记录数 |
| `CHART_MAX` | 300 | 图表内存缓冲最大点数 |
| `X_WINDOW` | 60 秒 | 图表 X 轴显示窗口 |

### 7.4 查询策略

```cpp
// 每次打开数据库或每 10 个数据点时刷新表格
void TrendPage::refreshHistoryTable()
{
    QSqlQuery query(m_db);
    query.exec(
        "SELECT timestamp, temperature, humidity "
        "FROM sensor_log ORDER BY id DESC LIMIT 100"
    );

    // 填充 QTableWidget (3 列: 时间 | 温度 | 湿度)
    int row = 0;
    while (query.next()) {
        qint64 ts = query.value(0).toLongLong();
        double temp = query.value(1).toDouble();
        double humi = query.value(2).toDouble();

        m_tableHistory->setItem(row, 0,
            new QTableWidgetItem(
                QDateTime::fromSecsSinceEpoch(ts).toString("MM-dd hh:mm:ss")
            ));
        m_tableHistory->setItem(row, 1,
            new QTableWidgetItem(QString::number(temp, 'f', 1) + " °C"));
        m_tableHistory->setItem(row, 2,
            new QTableWidgetItem(QString::number(humi, 'f', 1) + " %"));
        row++;
    }
}
```

---

## 8. 告警机制

### 8.1 告警架构

```
                    告警检测 / Alarm Detection
   ┌──────────────────────────────────────────────────────────┐
   │                                                          │
   │   ┌─────────────────────┐    ┌──────────────────────┐   │
   │   │  服务端告警          │    │  客户端本地告警       │   │
   │   │  gateway/alarm       │    │  (阈值自检)           │   │
   │   │  (MQTT QoS 1)       │    │                       │   │
   │   │                     │    │  if temp > tempMax    │   │
   │   │  embsky Cloud ─────►│    │     → Alarm!          │   │
   │   │  独立检测            │    │  if humi > humiMax    │   │
   │   │                     │    │     → Alarm!          │   │
   │   └────────┬────────────┘    └──────────┬───────────┘   │
   │            │                            │               │
   │            ▼                            ▼               │
   │   ┌──────────────────────────────────────────────┐     │
   │   │           AlarmPage 告警表格                   │     │
   │   │  ┌──────────────────────────────────────┐    │     │
   │   │  │ ID | Type | Limit | Actual | Status  │    │     │
   │   │  │ 42 | TempHi│ 40.0 | 55.3   | 🔴     │    │     │
   │   │  └──────────────────────────────────────┘    │     │
   │   │  状态指示器: 🟢 Normal / 🔴 ALARMING         │     │
   │   └──────────────────────────────────────────────┘     │
   │                                                          │
   └──────────────────────────────────────────────────────────┘
```

### 8.2 阈值同步流程

```
  AlarmPage UI               mainWidget            embsky Cloud
  ────────────              ───────────            ────────────
       │                         │                       │
       │ spinBox 值变化           │                       │
       │──thresholdsChanged─────>│                       │
       │                         │                       │
       │                更新本地阈值                       │
       │                m_tempMaxLimit                   │
       │                m_humiMaxLimit                   │
       │                         │                       │
       │                 PUBLISH gateway/thresholds/set ─>│
       │                {"temp_max":X,"humi_max":Y}      │
       │                         │                       │
       │                         │    Cloud 更新阈值      │
       │                         │    并 Retained 广播    │
       │                         │<── gateway/thresholds ─│
       │                         │   (Retained 消息)      │
       │                         │                       │
       │<──updateThresholds()────│                       │
       │   (同步给其他客户端)      │                       │
```

### 8.3 手动测试告警

```cpp
void mainWidget::onManualAlarmRequested()
{
    if (!isMqttConnected()) {
        QMessageBox::information(this,
            "提示", "请先连接 embsky Broker");
        return;
    }

    // 构造一个故意超过阈值的假数据包
    DeviceDataPacket testPacket;
    testPacket.dev_id = 1;
    testPacket.temperature = m_tempMaxLimit + 5.0;  // 超出阈值 5°C
    testPacket.humidity = 50.0;
    testPacket.timestamp = QDateTime::currentSecsSinceEpoch();

    processReceivedData(testPacket);  // 触发告警检测
}
```

---

## 9. 连接生命周期

### 9.1 状态机

```
                   ┌──────────┐
                   │  IDLE    │  (初始状态)
                   └────┬─────┘
                        │ connectToHost()
                        ▼
                   ┌──────────┐
              ┌───>│CONNECTING│
              │    └────┬─────┘
              │         │ TCP 握手 + CONNACK
              │         ▼
              │    ┌──────────┐     disconnectFromHost()
              │    │CONNECTED │─────────────────────────┐
              │    └────┬─────┘                         │
              │         │ 网络断开 / Broker 踢出         │
              │         ▼                               │
              │    ┌──────────────┐                     │
              └────│ DISCONNECTED │<────────────────────┘
                   └──────────────┘
                   (需手动重连)
```

### 9.2 连接成功回调

```cpp
void mainWidget::onMqttConnected()
{
    qDebug() << "embsky MQTT connected successfully";

    // 1. 更新所有页面的连接状态
    m_settingsPage->updateEmbskyConnectionStatus(true);
    m_overviewPage->setMqttConnected(true);
    m_trendPage->setMqttConnected(true);
    m_alarmPage->setMqttConnected(true);

    // 2. 重新订阅所有主题
    subscribeTopics();

    // 3. 请求历史告警记录
    requestAlarmLogs(0, QDateTime::currentSecsSinceEpoch(), 500);

    // 4. 阈值自动通过 Retained 消息获取 (gateway/thresholds)
}
```

### 9.3 断开回调

```cpp
void mainWidget::onMqttDisconnected()
{
    qDebug() << "embsky MQTT disconnected";

    // 所有页面标记为断开状态
    m_settingsPage->updateEmbskyConnectionStatus(false);
    m_overviewPage->setMqttConnected(false);
    m_trendPage->setMqttConnected(false);
    m_alarmPage->setMqttConnected(false);

    // ⚠️ 注意：没有自动重连逻辑，需要用户手动点击 Connect
}
```

### 9.4 析构清理

```cpp
mainWidget::~mainWidget()
{
    if (m_mqttClient && m_mqttClient->state() == QMqttClient::Connected) {
        m_mqttClient->disconnectFromHost();
    }
    delete m_mqttClient;
}
```

---

## 10. 错误处理

### 10.1 错误码映射

```cpp
void mainWidget::onMqttError(QMqttClient::ClientError error)
{
    QString msg;
    switch (error) {
    case QMqttClient::NoError:
        return;  // 无错误，不处理
    case QMqttClient::InvalidProtocolVersion:
        msg = "协议版本不匹配 (需要 MQTT 3.1.1)";
        break;
    case QMqttClient::IdRejected:
        msg = "Client ID 被拒绝 (请使用唯一ID)";
        break;
    case QMqttClient::ServerUnavailable:
        msg = "embsky 服务不可达 (检查地址和端口)";
        break;
    case QMqttClient::BadUsernameOrPassword:
        msg = "认证失败 (检查用户名和密码)";
        break;
    case QMqttClient::NotAuthorized:
        msg = "未授权访问";
        break;
    case QMqttClient::TransportInvalid:
        msg = "传输层错误 (检查 Broker 地址)";
        break;
    case QMqttClient::ProtocolViolation:
        msg = "协议违规 (MQTT 3.1.1 要求)";
        break;
    case QMqttClient::UnknownError:
        msg = "未知错误";
        break;
    case QMqttClient::Mqtt5SpecificError:
        msg = "MQTT 5.0 特定错误";
        break;
    }

    // 所有错误 (除 NoError) 都标记断开
    m_settingsPage->updateEmbskyConnectionStatus(false);
    m_overviewPage->setMqttConnected(false);
    m_trendPage->setMqttConnected(false);
    m_alarmPage->setMqttConnected(false);
}
```

### 10.2 异常场景处理

| 场景 | 处理方式 |
|------|----------|
| JSON 解析失败 | 尝试纯数字解析，仍失败则静默返回 |
| 连接时字段为空 | `QMessageBox::warning` 阻止连接 |
| 发布时未连接 | `isMqttConnected()` 检查，未连接则静默返回 |
| SQLite 打开失败 | `qWarning()` 日志警告 |
| 订阅失败 | 无显式错误处理 (建议添加) |

---

## 11. 最佳实践

### 11.1 ✅ 已实现的好实践

- **单例 MQTT 客户端** — 全应用一个 `QMqttClient`，避免资源浪费
- **QoS 分层** — 高频数据 QoS 0，关键消息 QoS 1
- **Retained 消息** — 阈值配置维持久化，新客户端即时同步
- **本地缓存** — 双传感器交叉缓存，保证 UI 数据完整性
- **本地 SQLite** — 离线数据不丢，在线查询加速
- **连接检查** — 发布前检查 `isMqttConnected()`
- **析构安全** — 销毁前主动断开连接

### 11.2 ⚠️ 可改进的点

| 问题 | 影响 | 建议方案 |
|------|------|----------|
| 无自动重连 | 网络抖动断开后需手动操作 | 添加指数退避自动重连 (1s→2s→4s→...max 60s) |
| 无心跳监控 | 无法及时发现静默断开 | 监听 `pingResponseReceived`，超时强制重连 |
| 无 TLS 加密 | 数据明文传输 | 生产环境启用 `QSslConfiguration` (port 8883) |
| 无离线缓冲 | 断连期间发布消息丢失 | 本地队列 + 重连后冲刷 |
| 无历史响应订阅 | `requestHistory()` 发出但无订阅 `gateway/history/resp` | 补充订阅和解析逻辑 |
| Client ID 可冲突 | 时间戳可能不唯一 | 添加 UUID 生成: `QUuid::createUuid().toString()` |

### 11.3 生产环境推荐配置

```cpp
// 推荐的生产环境 MQTT 配置
void mainWidget::setupProductionMqtt()
{
    m_mqttClient = new QMqttClient(this);
    m_mqttClient->setProtocolVersion(QMqttClient::MQTT_3_1_1);
    m_mqttClient->setCleanSession(false);          // 启用持久会话
    m_mqttClient->setKeepAlive(30);                 // 缩短心跳间隔

    // 启用 TLS
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    m_mqttClient->setSslConfiguration(sslConfig);
    m_mqttClient->setPort(8883);

    // Client ID 用 UUID 保证唯一
    m_mqttClient->setClientId(
        QUuid::createUuid().toString(QUuid::WithoutBraces)
    );

    // 监控心跳
    connect(m_mqttClient, &QMqttClient::pingResponseReceived,
            this, [this]() {
                m_lastPingTime = QDateTime::currentMSecsSinceEpoch();
            });

    // 自动重连
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);  // 5 秒后重试
    connect(m_mqttClient, &QMqttClient::disconnected,
            this, [this]() {
                if (m_shouldReconnect) {
                    m_reconnectTimer->start();
                }
            });
    connect(m_reconnectTimer, &QTimer::timeout,
            this, [this]() {
                m_mqttClient->connectToHost();
            });
}
```

---

## 12. 故障排查

### 12.1 常见问题

| 现象 | 可能原因 | 解决方法 |
|------|----------|----------|
| 连接超时 | 1. embsky 地址或端口错误<br>2. 防火墙阻止<br>3. 网络不通 | 1. 检查 Settings 页配置<br>2. `telnet iot.embsky.com 1883` 测试<br>3. 关闭防火墙重试 |
| 认证失败 | 用户名或密码错误 | 检查 embsky 平台账号密码 |
| Client ID 被拒 | 同一 ID 重复连接 | 清空 Client ID 字段，让系统自动生成 |
| 收不到数据 | 1. 未订阅主题<br>2. 传感器不在线<br>3. 主题名不匹配 | 1. 检查是否已连接 (绿色状态)<br>2. 用 MQTT.fx 等工具验证主题<br>3. 检查 Settings 页主题配置 |
| 仪表盘不更新 | 1. MQTT 消息 JSON 格式错误<br>2. 动画定时器未启动 | 1. 查看控制台日志<br>2. 检查 `processReceivedData` 调用链 |
| 趋势图无数据 | 1. SQLite 数据库文件不可写<br>2. 数据在 60 秒窗口外 | 1. 检查 `trend_data.db` 是否生成<br>2. 等待新数据或增大 `X_WINDOW` |

### 12.2 调试技巧

```cpp
// 在 mainWidget 构造函数中启用 MQTT 日志
m_mqttClient->setClientId("debug_client_" + QUuid::createUuid().toString());

// 添加全局消息监控
connect(m_mqttClient, &QMqttClient::messageReceived,
        this, [](const QByteArray &msg, const QMqttTopicName &topic) {
            qDebug() << "[MQTT DEBUG] Topic:" << topic.name()
                     << "Payload:" << msg;
        });

// 监控所有状态变化
connect(m_mqttClient, &QMqttClient::stateChanged,
        this, [](QMqttClient::ClientState state) {
            qDebug() << "[MQTT DEBUG] State:" << state;
        });
```

### 12.3 验证工具

推荐使用以下工具独立验证 embsky MQTT Broker:

```bash
# 使用 mosquitto_sub 验证传感器数据
mosquitto_sub -h iot.embsky.com -p 1883 \
    -u <username> -P <password> \
    -t "eslink/5800/5910/16790" -q 0

# 使用 mosquitto_pub 测试阈值下发
mosquitto_pub -h iot.embsky.com -p 1883 \
    -u <username> -P <password> \
    -t "gateway/thresholds/set" -q 1 \
    -m '{"temp_max":35.0,"humi_max":75.0}'

# 验证 Retained 阈值消息
mosquitto_sub -h iot.embsky.com -p 1883 \
    -u <username> -P <password> \
    -t "gateway/thresholds" -q 1 --retained-only
```

---

## 附录

### A. 关键文件索引

| 文件 | 内容 |
|------|------|
| `loginpro/mainwidget.cpp` | MQTT 客户端管理、订阅/发布、数据分发 |
| `loginpro/mainwidget.h` | MQTT 相关信号槽声明 |
| `loginpro/protocol.h` | `EmbskyConfig`, `DeviceDataPacket`, `AlarmNotificationRecord` |
| `loginpro/settingspage.cpp` | Broker 配置 UI (地址/端口/账号/主题) |
| `loginpro/overviewpage.cpp` | 模拟仪表盘 (MQTT 数据消费) |
| `loginpro/trendpage.cpp` | 趋势图 + SQLite 存储 |
| `loginpro/alarmpage.cpp` | 告警表格 + 阈值设置 UI |
| `loginpro/loginpro.pro` | 项目依赖: `QT += mqtt charts sql` |
| `loginpro/qtmqtt-6.11.1/` | QtMqtt 6.11.1 源码 |

### B. 依赖版本

| 依赖 | 版本 | 用途 |
|------|------|------|
| Qt | 6.11.1 | 应用框架 |
| QtMqtt | 6.11.1 | MQTT 客户端 |
| Qt Charts | 6.11.1 | 趋势图 |
| Qt SQL | 6.11.1 | SQLite 存储 |
| MinGW | 13.1.0 | Windows 编译链 |
| CMake | 3.30+ | QtMqtt 构建 |

### C. 协议参数汇总

| 参数 | 值 |
|------|:---|
| MQTT 版本 | **3.1.1** |
| 传输层 | **TCP (明文)** |
| 默认端口 | **1883** |
| Clean Session | **true** |
| Keep Alive | **60 秒** |
| 最大消息大小 | 未限制 |
| Will Message | 未使用 |
| TLS/SSL | 未启用 |

---

<div align="center">

```
  📡 MQTT 3.1.1  ·  🏭 QoS 0/1  ·  💾 SQLite Local  ·  ☁️ embsky Cloud
```

<br/>
<sub>IoT Edge Intelligence Gateway Monitoring System — MQTT Usage Guide</sub>
</div>
