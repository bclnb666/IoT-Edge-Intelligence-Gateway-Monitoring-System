#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>
#include <QTimer>
#include "protocol.h"

class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = nullptr);

    void updateSensorData(const DeviceDataPacket &packet);
    void updateConnectionStatus(bool connected);
    void updateThresholds(double tempMax, double humiMax);

    double tempMaxLimit() const { return m_tempMaxLimit; }
    double humiMaxLimit() const { return m_humiMaxLimit; }

signals:
    void thresholdsChanged(double tempMax, double humiMax);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_currentTemp;
    double m_currentHumi;
    double m_targetTemp;
    double m_targetHumi;
    double m_tempMaxLimit;
    double m_humiMaxLimit;
    bool m_isConnected;
    bool m_tempAlarm;
    bool m_humiAlarm;

    QTimer *m_animTimer;

    void drawGauge(QPainter &painter, const QRectF &rect,
                   double value, double minVal, double maxVal,
                   const QString &unit, const QString &label,
                   const QColor &startColor, const QColor &endColor,
                   bool isAlarm);
};

#endif // OVERVIEWPAGE_H
