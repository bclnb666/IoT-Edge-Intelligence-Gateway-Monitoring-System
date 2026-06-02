#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <QMetaType>
#include <QString>

#define SERVER_IP "10.12.153.13"
#define SERVER_PORT 9527

#define CNTSIZE 64
#define PWDSIZE 512

// 认证状态 / 请求类型
enum {
    LOGIN_SUCCESS,
    LOGIN_COUNT_NO_EXIST,
    LOGIN_ERROR_PASSWD,
    REGISTER_COUNT_EXIST,
    REGISTER_SUCCESS,
    REQ_LOGIN,
    REQ_REGISTER
};

// ============================================================
// 认证相关结构体（TCP :9527）
// ============================================================

struct login_st
{
    char count[CNTSIZE];
    char passwd[PWDSIZE];
    uint8_t login_state;
}__attribute((packed));

struct register_st
{
    char count[CNTSIZE];
    char passwd[PWDSIZE];
    uint8_t register_state;
}__attribute((packed));

// ============================================================
// MQTT 数据传输结构体
// ============================================================

#pragma pack(push, 1)
typedef struct {
    int dev_id;
    double temperature;
    double humidity;
    long long timestamp;
} DeviceDataPacket;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    int32_t  alarm_id;
    int32_t  dev_id;
    char     alarm_type[32];
    double   limit_value;
    double   actual_value;
    int32_t  status;
    int64_t  occur_time;
} AlarmNotificationRecord;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    int32_t  dev_id;
    char     var_name[32];
    double   var_value;
    int64_t  time_stamp;
} HistoryRecord;
#pragma pack(pop)

// ============================================================
// embsky 云平台 MQTT 配置
// ============================================================

struct EmbskyConfig {
    QString brokerHost;   // MQTT 地址 (默认 iot.embsky.com)
    quint16 brokerPort;   // MQTT 端口 (默认 1883)
    QString username;     // MQTT 用户名
    QString password;     // MQTT 密码
    QString clientId;     // MQTT 客户端ID
    quint16 keepAlive;    // 保活间隔秒 (默认 60)
    QString tempTopic;    // 温度传感器 Topic (如: eslink/5800/5910/16790)
    QString humiTopic;    // 湿度传感器 Topic (如: eslink/5800/5910/16789)
};

// Qt 元类型注册
Q_DECLARE_METATYPE(DeviceDataPacket)
Q_DECLARE_METATYPE(AlarmNotificationRecord)
Q_DECLARE_METATYPE(HistoryRecord)
Q_DECLARE_METATYPE(EmbskyConfig)

#endif // PROTOCOL_H
