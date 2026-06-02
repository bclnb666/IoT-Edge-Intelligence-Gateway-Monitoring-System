#ifndef TRENDPAGE_H
#define TRENDPAGE_H

#include <QWidget>
#include <QVector>
#include <QSqlDatabase>
#include "protocol.h"

class QChart;
class QChartView;
class QSplineSeries;
class QLineSeries;
class QAreaSeries;
class QCategoryAxis;
class QValueAxis;
class QLabel;
class QTableWidget;

class TrendPage : public QWidget
{
    Q_OBJECT

public:
    explicit TrendPage(QWidget *parent = nullptr);
    ~TrendPage() override;

    void updateSensorData(const DeviceDataPacket &packet);
    void updateConnectionStatus(bool connected);
    void onHistoryDataReceived(const QVector<HistoryRecord> &records);

private slots:
    void onTempHovered(const QPointF &point, bool state);
    void onHumiHovered(const QPointF &point, bool state);

private:
    // ---- Chart components ----
    QChart        *m_chart;
    QChartView    *m_view;
    QSplineSeries *m_tempSeries;     // 温度曲线 (upper)
    QLineSeries   *m_tempBaseline;   // 温度填充基线 y=0
    QAreaSeries   *m_tempArea;       // 温度渐变面积
    QSplineSeries *m_humiSeries;     // 湿度曲线
    QCategoryAxis *m_axisX;          // 倒推时间轴 "Xs前"
    QValueAxis    *m_axisYTemp;      // 左 Y 轴 温度
    QValueAxis    *m_axisYHumi;      // 右 Y 轴 湿度

    // ---- UI overlays ----
    QLabel *m_lblTempVal;            // 悬浮温度值
    QLabel *m_lblHumiVal;            // 悬浮湿度值
    QLabel *m_tooltip;               // 悬浮提示

    // ---- Table ----
    QTableWidget *m_table;

    // ---- SQLite ----
    QSqlDatabase m_db;

    // ---- Data buffers ----
    QVector<qreal> m_bufSecAbs;      // 绝对秒（从首个数据点起算）
    QVector<qreal> m_bufTemp;
    QVector<qreal> m_bufHumi;
    qint64 m_startTimestamp;         // 首个数据点的 Unix 时间戳

    static constexpr int   CHART_MAX  = 300;
    static constexpr int   DB_MAX     = 1000;
    static constexpr qreal X_WINDOW   = 60.0;   // 60 秒滚动窗口

    // ---- Internal helpers ----
    void setupChart();
    void setupTable();
    void initDatabase();
    void storeDataPoint(qint64 ts, double temp, double humi);
    void refreshHistoryTable();
    void flushSeries();
    void updateXAxisLabels(qreal latestAbsSec);
    void updateFloatingLabels(double temp, double humi);
    void showTooltip(const QPointF &point, const QString &text);
    void hideTooltip();
};

#endif // TRENDPAGE_H
