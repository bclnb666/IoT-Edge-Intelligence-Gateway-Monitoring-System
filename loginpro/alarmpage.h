#ifndef ALARMPAGE_H
#define ALARMPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QVector>
#include "protocol.h"

class AlarmPage : public QWidget
{
    Q_OBJECT

public:
    explicit AlarmPage(QWidget *parent = nullptr);

    void updateSensorData(const DeviceDataPacket &packet);
    void updateConnectionStatus(bool connected);
    void updateThresholds(double tempMax, double humiMax);
    void restoreFromDatabase();

    // 服务器报警通知和查询响应处理
    void onAlarmNotificationReceived(const AlarmNotificationRecord &notif);
    void onAlarmDataReceived(int requestId, const QVector<AlarmNotificationRecord> &records);

    double tempMaxLimit() const;
    double humiMaxLimit() const;

signals:
    void thresholdsChanged(double tempMax, double humiMax);
    void manualAlarmRequested();
    void clearHistoryRequested();

private:
    QTableWidget *m_tableHistory;
    QTableWidget *m_tableAlarms;
    QDoubleSpinBox *m_spinTempLimit;
    QDoubleSpinBox *m_spinHumiLimit;
    QLabel *m_lblAlarmStatus;
    QLabel *m_lblAlarmCount;

    int m_alarmCounter;

    double m_tempMaxLimit;
    double m_humiMaxLimit;
    double m_currentTemp;
    double m_currentHumi;

    void triggerAlarm(const QString &alarmType, double limitVal, double actualVal);
    void setupUI();
};

#endif // ALARMPAGE_H
