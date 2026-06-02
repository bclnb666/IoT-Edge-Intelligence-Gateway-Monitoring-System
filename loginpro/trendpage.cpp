#include "trendpage.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <QChart>
#include <QChartView>
#include <QSplineSeries>
#include <QLineSeries>
#include <QAreaSeries>
#include <QCategoryAxis>
#include <QValueAxis>
#include <QLinearGradient>
#include <QEasingCurve>

// =====================================================================
// 构造 / 析构
// =====================================================================

TrendPage::TrendPage(QWidget *parent)
    : QWidget(parent), m_startTimestamp(0)
{
    setStyleSheet("background-color: #1E1E2F;");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // ---------- 上半：图表 ----------
    setupChart();
    mainLayout->addWidget(m_view, 2);

    // ---------- 下半：历史表格 ----------
    auto *tableWrap = new QVBoxLayout();
    auto *lblTitle = new QLabel("历史记录", this);
    lblTitle->setStyleSheet(
        "color: #00D2D3; font-size: 13px; font-weight: bold; "
        "font-family: 'Microsoft YaHei'; padding: 2px 4px; background: transparent;");
    tableWrap->addWidget(lblTitle);

    setupTable();
    tableWrap->addWidget(m_table, 1);
    mainLayout->addLayout(tableWrap, 1);

    initDatabase();

    // ---------- 悬浮数值标签 (图表右上角) ----------
    m_lblTempVal = new QLabel(m_view);
    m_lblTempVal->setStyleSheet(
        "color: #00FF00; font-size: 14px; font-weight: bold; "
        "font-family: 'Consolas', 'Microsoft YaHei'; background: transparent;");
    m_lblHumiVal = new QLabel(m_view);
    m_lblHumiVal->setStyleSheet(
        "color: #A020F0; font-size: 14px; font-weight: bold; "
        "font-family: 'Consolas', 'Microsoft YaHei'; background: transparent;");

    // ---------- 悬浮 tooltip ----------
    m_tooltip = new QLabel(m_view);
    m_tooltip->setStyleSheet(
        "background-color: rgba(30,30,47,230); color: white; "
        "padding: 6px 10px; border: 1px solid #576574; border-radius: 4px; "
        "font-size: 12px; font-family: 'Microsoft YaHei';");
    m_tooltip->hide();
    m_tooltip->setAttribute(Qt::WA_TransparentForMouseEvents);
}

TrendPage::~TrendPage()
{
    if (m_db.isOpen()) m_db.close();
}

// =====================================================================
// 图表构建
// =====================================================================

void TrendPage::setupChart()
{
    // ---- 温度平滑曲线 (绿色) ----
    m_tempSeries = new QSplineSeries();
    QPen tempPen(QColor("#00FF00"), 2.5);
    tempPen.setCapStyle(Qt::RoundCap);
    tempPen.setJoinStyle(Qt::RoundJoin);
    m_tempSeries->setPen(tempPen);

    // ---- 温度面积填充基线 y=0 ----
    m_tempBaseline = new QLineSeries();
    m_tempBaseline->setPen(QPen(Qt::transparent));

    // ---- 绿色 → 透明 渐变填充 ----
    m_tempArea = new QAreaSeries(m_tempSeries, m_tempBaseline);
    QLinearGradient grad(QPointF(0, 0), QPointF(0, 1));
    grad.setColorAt(0.0, QColor(0, 255, 0, 70));
    grad.setColorAt(0.5, QColor(0, 255, 0, 20));
    grad.setColorAt(1.0, QColor(0, 255, 0, 0));
    grad.setCoordinateMode(QGradient::ObjectMode);
    m_tempArea->setBrush(grad);
    m_tempArea->setPen(QPen(Qt::transparent));

    // ---- 湿度平滑曲线 (紫色) ----
    m_humiSeries = new QSplineSeries();
    QPen humiPen(QColor("#A020F0"), 2.5);
    humiPen.setCapStyle(Qt::RoundCap);
    humiPen.setJoinStyle(Qt::RoundJoin);
    m_humiSeries->setPen(humiPen);

    // ---- QChart ----
    m_chart = new QChart();
    m_chart->addSeries(m_tempArea);    // 面积最底层
    m_chart->addSeries(m_tempSeries);  // 温度曲线在上
    m_chart->addSeries(m_humiSeries);  // 湿度曲线

    m_chart->setTitle("实时趋势监控");
    m_chart->setTitleFont(QFont("Microsoft YaHei", 13, QFont::Bold));
    m_chart->setTitleBrush(QBrush(QColor("#00D2D3")));
    m_chart->setBackgroundBrush(QBrush(Qt::transparent));
    m_chart->setBackgroundRoundness(0);

    // 绘图区背景
    m_chart->setPlotAreaBackgroundBrush(QBrush(QColor("#252540")));
    m_chart->setPlotAreaBackgroundVisible(true);

    // 图例
    m_chart->legend()->setLabelColor(QColor("#CCCCCC"));
    m_chart->legend()->setAlignment(Qt::AlignTop);
    m_chart->legend()->setFont(QFont("Microsoft YaHei", 10));
    m_chart->setMargins(QMargins(6, 6, 6, 6));

    // 动画
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->setAnimationDuration(300);
    m_chart->setAnimationEasingCurve(QEasingCurve::OutCubic);

    // ---- X 轴：倒推时间 (QCategoryAxis) ----
    m_axisX = new QCategoryAxis();
    m_axisX->setRange(0, X_WINDOW);
    m_axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    m_axisX->setGridLineColor(QColor("#3A3A50"));
    m_axisX->setLinePenColor(QColor("#555566"));
    m_axisX->setLabelsColor(QColor("#AAAAAA"));
    m_axisX->setLabelsFont(QFont("Microsoft YaHei", 9));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_tempArea->attachAxis(m_axisX);
    m_tempSeries->attachAxis(m_axisX);
    m_humiSeries->attachAxis(m_axisX);

    // ---- 左 Y 轴：温度 ----
    m_axisYTemp = new QValueAxis();
    m_axisYTemp->setTitleText("温度 (°C)");
    m_axisYTemp->setTitleBrush(QBrush(QColor("#00FF00")));
    m_axisYTemp->setLabelsColor(QColor("#00FF00"));
    m_axisYTemp->setGridLineColor(QColor("#3A3A50"));
    m_axisYTemp->setLinePenColor(QColor("#00FF00"));
    m_axisYTemp->setRange(35, 60);
    m_chart->addAxis(m_axisYTemp, Qt::AlignLeft);
    m_tempArea->attachAxis(m_axisYTemp);
    m_tempSeries->attachAxis(m_axisYTemp);

    // ---- 右 Y 轴：湿度 ----
    m_axisYHumi = new QValueAxis();
    m_axisYHumi->setTitleText("湿度 (%)");
    m_axisYHumi->setTitleBrush(QBrush(QColor("#A020F0")));
    m_axisYHumi->setLabelsColor(QColor("#A020F0"));
    m_axisYHumi->setGridLineColor(QColor(0, 0, 0, 0));
    m_axisYHumi->setLinePenColor(QColor("#A020F0"));
    m_axisYHumi->setRange(0, 100);
    m_chart->addAxis(m_axisYHumi, Qt::AlignRight);
    m_humiSeries->attachAxis(m_axisYHumi);

    // ---- ChartView：多重抗锯齿 ----
    m_view = new QChartView(m_chart);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_view->setRubberBand(QChartView::NoRubberBand);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setInteractive(true);
    m_view->setStyleSheet("background: transparent; border: none;");

    // Hover
    connect(m_tempSeries, &QSplineSeries::hovered, this, &TrendPage::onTempHovered);
    connect(m_humiSeries, &QSplineSeries::hovered, this, &TrendPage::onHumiHovered);
}

// =====================================================================
// 表格构建
// =====================================================================

void TrendPage::setupTable()
{
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"时间", "温度 (°C)", "湿度 (%)"});

    m_table->setStyleSheet(
        "QTableWidget {"
        "  background-color: #252540; color: #CCCCCC;"
        "  border: 1px solid #3A3A50; gridline-color: #3A3A50;"
        "  font-size: 12px; font-family: 'Microsoft YaHei';"
        "  selection-background-color: #00D2D3; selection-color: #1E1E2F;"
        "  outline: none; }"
        "QTableWidget::item { padding: 3px 6px; border: none; }"
        "QTableWidget::item:selected { background-color: #00D2D3; color: #1E1E2F; }"
        "QTableWidget::item:alternate { background-color: #2B2B3F; }"
        "QHeaderView::section {"
        "  background-color: #2F3545; color: #00D2D3;"
        "  padding: 4px 8px; border: 1px solid #3A3A50;"
        "  font-weight: bold; font-size: 12px; }");

    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setShowGrid(true);
    m_table->setFocusPolicy(Qt::NoFocus);

    auto *hdr = m_table->horizontalHeader();
    hdr->setStretchLastSection(true);
    hdr->setSectionResizeMode(0, QHeaderView::Stretch);
    hdr->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
}

// =====================================================================
// SQLite 本地存储 (最多 1000 条)
// =====================================================================

void TrendPage::initDatabase()
{
    if (QSqlDatabase::contains("trend_history"))
        m_db = QSqlDatabase::database("trend_history");
    else
        m_db = QSqlDatabase::addDatabase("QSQLITE", "trend_history");
    m_db.setDatabaseName("trend_data.db");
    if (!m_db.open()) { qWarning() << "DB:" << m_db.lastError().text(); return; }

    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS sensor_log ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "timestamp INTEGER NOT NULL, temperature REAL, humidity REAL)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_ts ON sensor_log(timestamp)");
    refreshHistoryTable();
}

void TrendPage::storeDataPoint(qint64 ts, double temp, double humi)
{
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO sensor_log (timestamp, temperature, humidity) VALUES (?,?,?)");
    q.addBindValue(ts); q.addBindValue(temp); q.addBindValue(humi);
    q.exec();

    q.exec("SELECT COUNT(*) FROM sensor_log");
    if (q.next() && q.value(0).toInt() > DB_MAX) {
        int excess = q.value(0).toInt() - DB_MAX;
        q.exec(QString("DELETE FROM sensor_log WHERE id IN "
                       "(SELECT id FROM sensor_log ORDER BY id ASC LIMIT %1)").arg(excess));
    }
}

void TrendPage::refreshHistoryTable()
{
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    q.exec("SELECT timestamp, temperature, humidity FROM sensor_log ORDER BY id DESC LIMIT 100");

    m_table->setRowCount(0);
    for (int row = 0; q.next(); ++row) {
        m_table->insertRow(row);
        qint64 ts  = q.value(0).toLongLong();
        double tmp = q.value(1).toDouble();
        double hum = q.value(2).toDouble();

        auto *c0 = new QTableWidgetItem(
            QDateTime::fromSecsSinceEpoch(ts).toString("MM-dd hh:mm:ss"));
        c0->setForeground(QColor("#AAAAAA"));
        m_table->setItem(row, 0, c0);

        auto *c1 = new QTableWidgetItem(QString::number(tmp, 'f', 1));
        c1->setForeground(QColor("#00FF00"));
        m_table->setItem(row, 1, c1);

        auto *c2 = new QTableWidgetItem(QString::number(hum, 'f', 1));
        c2->setForeground(QColor("#A020F0"));
        m_table->setItem(row, 2, c2);
    }
}

// =====================================================================
// 悬浮提示
// =====================================================================

void TrendPage::onTempHovered(const QPointF &point, bool state)
{
    if (state) showTooltip(point, QString("温度  %1 °C").arg(point.y(), 0, 'f', 2));
    else      hideTooltip();
}

void TrendPage::onHumiHovered(const QPointF &point, bool state)
{
    if (state) showTooltip(point, QString("湿度  %1 %").arg(point.y(), 0, 'f', 2));
    else      hideTooltip();
}

void TrendPage::showTooltip(const QPointF &point, const QString &text)
{
    m_tooltip->setText(text); m_tooltip->adjustSize();
    QPointF sp = m_chart->mapToPosition(point);
    QPointF vp = m_view->mapFromScene(sp);
    int x = qMin((int)vp.x() + 16, m_view->width() - m_tooltip->width() - 4);
    int y = (int)vp.y() - m_tooltip->height() - 12;
    if (y < 4) y = (int)vp.y() + 16;
    m_tooltip->move(x, y);
    m_tooltip->show(); m_tooltip->raise();
}

void TrendPage::hideTooltip() { m_tooltip->hide(); }

// =====================================================================
// 右上角悬浮数值
// =====================================================================

void TrendPage::updateFloatingLabels(double temp, double humi)
{
    m_lblTempVal->setText(QString("温度: %1°C").arg(temp, 0, 'f', 2));
    m_lblHumiVal->setText(QString("湿度: %1%").arg(humi, 0, 'f', 2));
    m_lblTempVal->adjustSize();
    m_lblHumiVal->adjustSize();
    int x = m_view->width() - qMax(m_lblTempVal->width(), m_lblHumiVal->width()) - 12;
    m_lblTempVal->move(x, 4);
    m_lblHumiVal->move(x, 4 + m_lblTempVal->height() + 2);
}

// =====================================================================
// X 轴倒推标签: "50s前" "42s前" ... "0s前"
// =====================================================================

void TrendPage::updateXAxisLabels(qreal latestAbsSec)
{
    // 清除旧标签
    const auto labels = m_axisX->categoriesLabels();
    for (const auto &label : labels)
        m_axisX->remove(label);

    // 7 个均匀分布的标签位置
    const int count = 9;
    for (int i = 0; i < count; ++i) {
        qreal secsAgo = X_WINDOW * (count - 1 - i) / (count - 1);
        qreal xPos = latestAbsSec - secsAgo;
        if (xPos < 0) continue;
        m_axisX->append(QString::number((int)secsAgo) + "s前", xPos);
    }
}

// =====================================================================
// 实时数据入口
// =====================================================================

void TrendPage::updateSensorData(const DeviceDataPacket &packet)
{
    storeDataPoint(packet.timestamp, packet.temperature, packet.humidity);

    qint64 nowSec = QDateTime::currentSecsSinceEpoch();
    if (m_startTimestamp == 0) m_startTimestamp = nowSec;

    qreal absSec = (qreal)(nowSec - m_startTimestamp);   // 首点 = 0，最新点逐渐增大

    // 压入缓冲区
    m_bufSecAbs.append(absSec);
    m_bufTemp.append(packet.temperature);
    m_bufHumi.append(packet.humidity);

    // 丢弃超出窗口的旧数据
    while (!m_bufSecAbs.isEmpty() && (absSec - m_bufSecAbs.first()) > X_WINDOW) {
        m_bufSecAbs.removeFirst();
        m_bufTemp.removeFirst();
        m_bufHumi.removeFirst();
    }
    while (m_bufSecAbs.size() > CHART_MAX) {
        m_bufSecAbs.removeFirst();
        m_bufTemp.removeFirst();
        m_bufHumi.removeFirst();
    }

    flushSeries();
    updateFloatingLabels(packet.temperature, packet.humidity);

    static int counter = 0;
    if (++counter % 10 == 0) refreshHistoryTable();
}

// =====================================================================
// 刷新图表 (原子替换，无闪烁)
// =====================================================================

void TrendPage::flushSeries()
{
    if (m_bufSecAbs.isEmpty()) return;

    QList<QPointF> tempPts, humiPts, basePts;
    tempPts.reserve(m_bufSecAbs.size());
    humiPts.reserve(m_bufSecAbs.size());
    basePts.reserve(m_bufSecAbs.size());

    for (int i = 0; i < m_bufSecAbs.size(); ++i) {
        qreal x = m_bufSecAbs[i];
        tempPts << QPointF(x, m_bufTemp[i]);
        humiPts << QPointF(x, m_bufHumi[i]);
        basePts << QPointF(x, 0.0);
    }

    m_tempSeries->replace(tempPts);
    m_tempBaseline->replace(basePts);
    m_humiSeries->replace(humiPts);

    // X 轴窗口滚动 + 倒推标签
    qreal latest = m_bufSecAbs.last();
    m_axisX->setRange(latest - X_WINDOW, latest);
    updateXAxisLabels(latest);

    // Y 轴动态范围
    double tMin = 40, tMax = 60, hMin = 0, hMax = 100;
    for (int i = 0; i < m_bufTemp.size(); ++i) {
        double t = m_bufTemp[i], h = m_bufHumi[i];
        if (t < tMin) tMin = t; if (t > tMax) tMax = t;
        if (h < hMin) hMin = h; if (h > hMax) hMax = h;
    }
    double tp = (tMax - tMin) * 0.2 + 2.0;
    m_axisYTemp->setRange(tMin - tp, tMax + tp);
    double hp = (hMax - hMin) * 0.2 + 2.0;
    m_axisYHumi->setRange(qMax(0.0, hMin - hp), qMin(100.0, hMax + hp));
}

// =====================================================================
// 连接状态
// =====================================================================

void TrendPage::updateConnectionStatus(bool connected)
{
    if (!connected) {
        m_tempSeries->clear();
        m_tempBaseline->clear();
        m_humiSeries->clear();
        m_bufSecAbs.clear();
        m_bufTemp.clear();
        m_bufHumi.clear();
        m_startTimestamp = 0;
    }
}

void TrendPage::onHistoryDataReceived(const QVector<HistoryRecord> &records) { Q_UNUSED(records); }
