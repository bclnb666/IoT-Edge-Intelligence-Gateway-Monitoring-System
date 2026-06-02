#include "overviewpage.h"
#include <QPainter>
#include <QtMath>
#include <QPainterPath>

OverviewPage::OverviewPage(QWidget *parent)
    : QWidget(parent)
    , m_currentTemp(0.0)
    , m_currentHumi(0.0)
    , m_targetTemp(0.0)
    , m_targetHumi(0.0)
    , m_tempMaxLimit(40.0)
    , m_humiMaxLimit(80.0)
    , m_isConnected(false)
    , m_tempAlarm(false)
    , m_humiAlarm(false)
{
    setMinimumSize(400, 300);
    setStyleSheet("background-color: #1E272E;");

    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(30);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        bool needUpdate = false;
        double speed = 0.08;
        if (qAbs(m_currentTemp - m_targetTemp) > 0.05) {
            m_currentTemp += (m_targetTemp - m_currentTemp) * speed;
            needUpdate = true;
        } else if (m_currentTemp != m_targetTemp) {
            m_currentTemp = m_targetTemp;
            needUpdate = true;
        }
        if (qAbs(m_currentHumi - m_targetHumi) > 0.05) {
            m_currentHumi += (m_targetHumi - m_currentHumi) * speed;
            needUpdate = true;
        } else if (m_currentHumi != m_targetHumi) {
            m_currentHumi = m_targetHumi;
            needUpdate = true;
        }
        if (needUpdate) update();
        if (!needUpdate && m_currentTemp == m_targetTemp && m_currentHumi == m_targetHumi)
            m_animTimer->stop();
    });
}

void OverviewPage::updateSensorData(const DeviceDataPacket &packet)
{
    m_targetTemp = packet.temperature;
    m_targetHumi = packet.humidity;
    m_tempAlarm = (m_targetTemp > m_tempMaxLimit);
    m_humiAlarm = (m_targetHumi > m_humiMaxLimit);

    if (!m_animTimer->isActive())
        m_animTimer->start();
}

void OverviewPage::updateConnectionStatus(bool connected)
{
    m_isConnected = connected;
    if (!connected) {
        m_targetTemp = 0.0;
        m_targetHumi = 0.0;
    }
    update();
}

void OverviewPage::updateThresholds(double tempMax, double humiMax)
{
    m_tempMaxLimit = tempMax;
    m_humiMaxLimit = humiMax;
    m_tempAlarm = (m_currentTemp > m_tempMaxLimit);
    m_humiAlarm = (m_currentHumi > m_humiMaxLimit);
    update();
}

void OverviewPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Title and status
    painter.setPen(QColor("#00D2D3"));
    QFont titleFont("Microsoft YaHei", 18, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(QRectF(0, 5, w, 35), Qt::AlignCenter, "工业仪表监控面板");

    // Connection status indicator
    painter.setFont(QFont("Microsoft YaHei", 11));
    painter.setPen(m_isConnected ? QColor("#2ECC71") : QColor("#E74C3C"));
    painter.drawText(QRectF(0, 38, w, 22), Qt::AlignCenter,
                     m_isConnected ? "● 已连接" : "● 未连接");

    // Calculate gauge areas
    double gaugeTop = 70;
    double gaugeHeight = h - gaugeTop - 10;
    double gaugeWidth = (w - 60) / 2.0;
    double gaugeSize = qMin(gaugeWidth, gaugeHeight) * 0.85;

    QRectF tempRect((w / 2.0 - gaugeSize) / 2.0, gaugeTop + (gaugeHeight - gaugeSize) / 2.0,
                    gaugeSize, gaugeSize);
    QRectF humiRect(w / 2.0 + (w / 2.0 - gaugeSize) / 2.0, gaugeTop + (gaugeHeight - gaugeSize) / 2.0,
                    gaugeSize, gaugeSize);

    drawGauge(painter, tempRect, m_currentTemp, -10.0, 60.0,
              "°C", "温度", QColor("#3498DB"), QColor("#E74C3C"), m_tempAlarm);
    drawGauge(painter, humiRect, m_currentHumi, 0.0, 100.0,
              "%", "湿度", QColor("#8B4513"), QColor("#3498DB"), m_humiAlarm);
}

void OverviewPage::drawGauge(QPainter &painter, const QRectF &rect,
                              double value, double minVal, double maxVal,
                              const QString &unit, const QString &label,
                              const QColor &startColor, const QColor &endColor,
                              bool isAlarm)
{
    painter.save();
    double cx = rect.center().x();
    double cy = rect.center().y();
    double radius = rect.width() / 2.0;

    int startAngle = 135 * 16;   // Qt angles in 1/16 degree, 135 degrees
    int sweepAngle = 270 * 16;   // 270 degree sweep
    // Background arc
    QPen arcPen(QColor("#2F3542"), radius * 0.12);
    arcPen.setCapStyle(Qt::FlatCap);
    painter.setPen(arcPen);
    painter.drawArc(QRectF(cx - radius + arcPen.widthF() / 2, cy - radius + arcPen.widthF() / 2,
                            (radius - arcPen.widthF() / 2) * 2, (radius - arcPen.widthF() / 2) * 2),
                    startAngle, sweepAngle);

    // Active arc (colored)
    double fraction = qBound(0.0, (value - minVal) / (maxVal - minVal), 1.0);
    int activeSweep = static_cast<int>(sweepAngle * fraction);

    QColor activeColor;
    if (isAlarm) {
        activeColor = QColor("#E74C3C");
    } else {
        double t = fraction;
        activeColor = QColor(
            static_cast<int>(startColor.red() + (endColor.red() - startColor.red()) * t),
            static_cast<int>(startColor.green() + (endColor.green() - startColor.green()) * t),
            static_cast<int>(startColor.blue() + (endColor.blue() - startColor.blue()) * t)
        );
    }

    arcPen.setColor(activeColor);
    painter.setPen(arcPen);
    painter.drawArc(QRectF(cx - radius + arcPen.widthF() / 2, cy - radius + arcPen.widthF() / 2,
                            (radius - arcPen.widthF() / 2) * 2, (radius - arcPen.widthF() / 2) * 2),
                    startAngle, activeSweep);

    // Tick marks
    QPen tickPen(QColor("#AAAAAA"), 2);
    painter.setPen(tickPen);
    QFont tickFont("Consolas", radius * 0.06);
    painter.setFont(tickFont);

    int numTicks = 7;
    for (int i = 0; i < numTicks; i++) {
        double tickFrac = static_cast<double>(i) / (numTicks - 1);
        int angleDeg = 135 + static_cast<int>(270 * tickFrac);
        double rad = qDegreesToRadians(static_cast<double>(angleDeg));

        double innerR = radius * 0.72;
        double outerR = radius * 0.82;
        double x1 = cx + innerR * qCos(rad);
        double y1 = cy - innerR * qSin(rad);
        double x2 = cx + outerR * qCos(rad);
        double y2 = cy - outerR * qSin(rad);
        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));

        double labelR = radius * 0.62;
        double tickVal = minVal + (maxVal - minVal) * tickFrac;
        QString tickStr = QString::number(tickVal, 'f', 0);
        QRectF tickRect(cx + labelR * qCos(rad) - 20, cy - labelR * qSin(rad) - 10, 40, 20);
        painter.setPen(QColor("#CCCCCC"));
        painter.drawText(tickRect, Qt::AlignCenter, tickStr);
        painter.setPen(tickPen);
    }

    // Needle
    double needleAngle = 135.0 + 270.0 * fraction;
    double needleRad = qDegreesToRadians(needleAngle);
    double needleLen = radius * 0.65;

    QPainterPath needlePath;
    needlePath.moveTo(cx + (radius * 0.08) * qCos(needleRad + M_PI_2),
                      cy - (radius * 0.08) * qSin(needleRad + M_PI_2));
    needlePath.lineTo(cx + needleLen * qCos(needleRad),
                      cy - needleLen * qSin(needleRad));
    needlePath.lineTo(cx + (radius * 0.08) * qCos(needleRad - M_PI_2),
                      cy - (radius * 0.08) * qSin(needleRad - M_PI_2));
    needlePath.closeSubpath();

    painter.setPen(Qt::NoPen);
    painter.setBrush(isAlarm ? QColor("#FF2E2E") : QColor("#00D2D3"));
    painter.drawPath(needlePath);

    // Center cap
    painter.setBrush(QColor("#2F3542"));
    painter.setPen(QPen(QColor("#576574"), 2));
    painter.drawEllipse(QPointF(cx, cy), radius * 0.08, radius * 0.08);

    // Value text
    QFont valFont("Consolas", radius * 0.14, QFont::Bold);
    painter.setFont(valFont);
    painter.setPen(isAlarm ? QColor("#FF2E2E") : QColor("#FFFFFF"));
    QRectF valRect(cx - radius * 0.4, cy + radius * 0.25, radius * 0.8, radius * 0.2);
    painter.drawText(valRect, Qt::AlignCenter, QString("%1%2").arg(value, 0, 'f', 1).arg(unit));

    // Label
    QFont labelFont("Microsoft YaHei", radius * 0.08);
    painter.setFont(labelFont);
    painter.setPen(QColor("#AAAAAA"));
    QRectF labelRect(cx - radius * 0.4, cy + radius * 0.42, radius * 0.8, radius * 0.15);
    painter.drawText(labelRect, Qt::AlignCenter, label);

    // Alarm indicator
    if (isAlarm) {
        QFont alarmFont("Microsoft YaHei", radius * 0.07, QFont::Bold);
        painter.setFont(alarmFont);
        painter.setPen(QColor("#FF2E2E"));
        QRectF alarmRect(cx - radius * 0.4, cy - radius * 0.55, radius * 0.8, radius * 0.12);
        painter.drawText(alarmRect, Qt::AlignCenter, "⚠ ALARM");
    }

    painter.restore();
}
