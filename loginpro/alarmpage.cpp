#include "alarmpage.h"
#include "mainwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>

AlarmPage::AlarmPage(QWidget *parent)
    : QWidget(parent)
    , m_alarmCounter(0)
    , m_tempMaxLimit(40.0)
    , m_humiMaxLimit(80.0)
    , m_currentTemp(0.0)
    , m_currentHumi(0.0)
{
    setupUI();
}

void AlarmPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    QString groupStyle =
        "QGroupBox { font-weight: bold; font-size: 14px; color: #00D2D3; "
        "border: 1px solid #576574; border-radius: 8px; margin-top: 15px; padding: 12px; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }";

    // --- Threshold Configuration ---
    QGroupBox *boxConfig = new QGroupBox("阈值配置", this);
    boxConfig->setStyleSheet(groupStyle);
    QVBoxLayout *configLayout = new QVBoxLayout(boxConfig);

    QHBoxLayout *tempRow = new QHBoxLayout();
    QLabel *lblTemp = new QLabel("温度上限 (°C):", this);
    lblTemp->setStyleSheet("font-weight: normal; color: #FF7675;");
    m_spinTempLimit = new QDoubleSpinBox(this);
    m_spinTempLimit->setRange(0, 100);
    m_spinTempLimit->setValue(m_tempMaxLimit);
    m_spinTempLimit->setDecimals(1);
    m_spinTempLimit->setStyleSheet("background-color: #2F3542; color: white; padding: 6px; border-radius: 4px; font-size: 14px;");
    tempRow->addWidget(lblTemp);
    tempRow->addWidget(m_spinTempLimit);
    tempRow->addStretch();
    configLayout->addLayout(tempRow);

    QHBoxLayout *humiRow = new QHBoxLayout();
    QLabel *lblHumi = new QLabel("湿度上限 (%):", this);
    lblHumi->setStyleSheet("font-weight: normal; color: #54A0FF;");
    m_spinHumiLimit = new QDoubleSpinBox(this);
    m_spinHumiLimit->setRange(0, 100);
    m_spinHumiLimit->setValue(m_humiMaxLimit);
    m_spinHumiLimit->setDecimals(1);
    m_spinHumiLimit->setStyleSheet("background-color: #2F3542; color: white; padding: 6px; border-radius: 4px; font-size: 14px;");
    humiRow->addWidget(lblHumi);
    humiRow->addWidget(m_spinHumiLimit);
    humiRow->addStretch();
    configLayout->addLayout(humiRow);

    connect(m_spinTempLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        m_tempMaxLimit = v;
        emit thresholdsChanged(m_tempMaxLimit, m_humiMaxLimit);
    });
    connect(m_spinHumiLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        m_humiMaxLimit = v;
        emit thresholdsChanged(m_tempMaxLimit, m_humiMaxLimit);
    });

    mainLayout->addWidget(boxConfig);

    // --- Alarm Status ---
    QGroupBox *boxStatus = new QGroupBox("报警状态", this);
    boxStatus->setStyleSheet(groupStyle);
    QVBoxLayout *statusLayout = new QVBoxLayout(boxStatus);

    QHBoxLayout *statusRow = new QHBoxLayout();
    m_lblAlarmStatus = new QLabel("● 正常", this);
    m_lblAlarmStatus->setStyleSheet("font-size: 16px; font-weight: bold; color: #2ECC71;");
    m_lblAlarmCount = new QLabel("报警总数: 0", this);
    m_lblAlarmCount->setStyleSheet("font-size: 14px; color: #AAAAAA;");
    statusRow->addWidget(m_lblAlarmStatus);
    statusRow->addStretch();
    statusRow->addWidget(m_lblAlarmCount);
    statusLayout->addLayout(statusRow);

    mainLayout->addWidget(boxStatus);

    // --- Action Buttons ---
    QHBoxLayout *btnLayout = new QHBoxLayout();

    QPushButton *btnManualAlarm = new QPushButton("突发高温测试", this);
    btnManualAlarm->setStyleSheet(
        "QPushButton { background-color: #EA2027; color: white; font-weight: bold; "
        "border-radius: 5px; padding: 10px 20px; font-size: 14px; } "
        "QPushButton:hover { background-color: #FF2E2E; }");
    connect(btnManualAlarm, &QPushButton::clicked, this, &AlarmPage::manualAlarmRequested);
    btnLayout->addWidget(btnManualAlarm);

    QPushButton *btnClear = new QPushButton("清空历史", this);
    btnClear->setStyleSheet(
        "QPushButton { background-color: #57606F; color: white; border-radius: 5px; "
        "padding: 10px 20px; font-size: 14px; } "
        "QPushButton:hover { background-color: #747D8C; }");
    connect(btnClear, &QPushButton::clicked, this, [this]() {
        m_tableHistory->setRowCount(0);
        m_tableAlarms->setRowCount(0);
        m_alarmCounter = 0;
        m_lblAlarmCount->setText("报警总数: 0");
        m_lblAlarmStatus->setText("● 正常");
        m_lblAlarmStatus->setStyleSheet("font-size: 16px; font-weight: bold; color: #2ECC71;");
        emit clearHistoryRequested();
    });
    btnLayout->addWidget(btnClear);

    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // --- History Table ---
    QGroupBox *boxHistory = new QGroupBox("数据记录", this);
    boxHistory->setStyleSheet(groupStyle);
    QVBoxLayout *histLayout = new QVBoxLayout(boxHistory);

    m_tableHistory = new QTableWidget(this);
    m_tableHistory->setColumnCount(4);
    m_tableHistory->setHorizontalHeaderLabels({"设备ID", "指标", "数值", "时间"});
    m_tableHistory->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableHistory->setStyleSheet(
        "QTableWidget { background-color: #2F3542; border: none; gridline-color: #475569; } "
        "QHeaderView::section { background-color: #57606F; color: white; padding: 4px; border: none; }");
    m_tableHistory->setEditTriggers(QAbstractItemView::NoEditTriggers);
    histLayout->addWidget(m_tableHistory);
    mainLayout->addWidget(boxHistory, 2);

    // --- Alarm Table ---
    QGroupBox *boxAlarms = new QGroupBox("报警记录", this);
    boxAlarms->setStyleSheet(groupStyle);
    QVBoxLayout *alarmLayout = new QVBoxLayout(boxAlarms);

    m_tableAlarms = new QTableWidget(this);
    m_tableAlarms->setColumnCount(5);
    m_tableAlarms->setHorizontalHeaderLabels({"ID", "报警类型", "上限值", "实测值", "触发时间"});
    m_tableAlarms->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableAlarms->setStyleSheet(m_tableHistory->styleSheet());
    m_tableAlarms->setEditTriggers(QAbstractItemView::NoEditTriggers);
    alarmLayout->addWidget(m_tableAlarms);
    mainLayout->addWidget(boxAlarms, 1);
}

void AlarmPage::updateSensorData(const DeviceDataPacket &packet)
{
    m_currentTemp = packet.temperature;
    m_currentHumi = packet.humidity;

    // Insert history rows
    QDateTime dt = QDateTime::fromSecsSinceEpoch(packet.timestamp);
    auto insertRow = [&](const QString &type, const QString &value) {
        m_tableHistory->insertRow(0);
        m_tableHistory->setItem(0, 0, new QTableWidgetItem(QString::number(packet.dev_id)));
        m_tableHistory->setItem(0, 1, new QTableWidgetItem(type));
        m_tableHistory->setItem(0, 2, new QTableWidgetItem(value));
        m_tableHistory->setItem(0, 3, new QTableWidgetItem(dt.toString("hh:mm:ss")));
    };

    insertRow("温度", QString("%1 °C").arg(m_currentTemp, 0, 'f', 2));
    insertRow("湿度", QString("%1 %").arg(m_currentHumi, 0, 'f', 2));

    while (m_tableHistory->rowCount() > 100)
        m_tableHistory->removeRow(m_tableHistory->rowCount() - 1);

    // Alarm checking
    bool hasAlarm = false;
    if (m_currentTemp > m_tempMaxLimit) {
        hasAlarm = true;
        triggerAlarm("温度过高", m_tempMaxLimit, m_currentTemp);
    }
    if (m_currentHumi > m_humiMaxLimit) {
        hasAlarm = true;
        triggerAlarm("湿度过高", m_humiMaxLimit, m_currentHumi);
    }

    if (hasAlarm) {
        m_lblAlarmStatus->setText("⚠ 报警中");
        m_lblAlarmStatus->setStyleSheet("font-size: 16px; font-weight: bold; color: #FF2E2E;");
    }
}

void AlarmPage::triggerAlarm(const QString &alarmType, double limitVal, double actualVal)
{
    // 仅更新本地 UI 显示（报警存储由 Linux 服务器端负责）
    m_alarmCounter++;

    m_tableAlarms->insertRow(0);
    m_tableAlarms->setItem(0, 0, new QTableWidgetItem(QString::number(m_alarmCounter)));

    QTableWidgetItem *typeItem = new QTableWidgetItem(alarmType);
    typeItem->setForeground(QBrush(QColor(255, 46, 46)));
    m_tableAlarms->setItem(0, 1, typeItem);

    m_tableAlarms->setItem(0, 2, new QTableWidgetItem(QString::number(limitVal, 'f', 1)));
    m_tableAlarms->setItem(0, 3, new QTableWidgetItem(QString::number(actualVal, 'f', 1)));
    m_tableAlarms->setItem(0, 4, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));

    while (m_tableAlarms->rowCount() > 50)
        m_tableAlarms->removeRow(m_tableAlarms->rowCount() - 1);

    m_lblAlarmCount->setText(QString("报警总数: %1").arg(m_alarmCounter));
}

void AlarmPage::restoreFromDatabase()
{
    // 从 MQTT 服务器请求历史报警记录（在 onMqttConnected 时由 mainWidget 调用）
    mainWidget *mw = qobject_cast<mainWidget*>(parent());
    if (mw && mw->isMqttConnected()) {
        mw->requestAlarmLogs(0, QDateTime::currentSecsSinceEpoch(), 500);
    }
}

// ============================================================
// 服务器端报警通知处理
// ============================================================

void AlarmPage::onAlarmNotificationReceived(const AlarmNotificationRecord &notif)
{
    m_alarmCounter++;

    m_tableAlarms->insertRow(0);
    m_tableAlarms->setItem(0, 0, new QTableWidgetItem(QString::number(notif.alarm_id)));
    m_tableAlarms->setItem(0, 1, new QTableWidgetItem(QString::fromUtf8(notif.alarm_type)));
    m_tableAlarms->setItem(0, 2, new QTableWidgetItem(QString::number(notif.limit_value, 'f', 1)));
    m_tableAlarms->setItem(0, 3, new QTableWidgetItem(QString::number(notif.actual_value, 'f', 1)));
    m_tableAlarms->setItem(0, 4, new QTableWidgetItem(
        QDateTime::fromSecsSinceEpoch(notif.occur_time).toString("yyyy-MM-dd hh:mm:ss")));

    while (m_tableAlarms->rowCount() > 50)
        m_tableAlarms->removeRow(m_tableAlarms->rowCount() - 1);

    m_lblAlarmCount->setText(QString("报警总数: %1").arg(m_alarmCounter));
    m_lblAlarmStatus->setText("⚠ 报警中");
    m_lblAlarmStatus->setStyleSheet("font-size: 16px; font-weight: bold; color: #FF2E2E;");
}

void AlarmPage::onAlarmDataReceived(int requestId, const QVector<AlarmNotificationRecord> &records)
{
    Q_UNUSED(requestId);
    m_tableAlarms->setRowCount(0);
    m_alarmCounter = records.size();
    for (int i = 0; i < records.size(); i++) {
        const auto &r = records[i];
        m_tableAlarms->insertRow(i);
        m_tableAlarms->setItem(i, 0, new QTableWidgetItem(QString::number(r.alarm_id)));
        m_tableAlarms->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(r.alarm_type)));
        m_tableAlarms->setItem(i, 2, new QTableWidgetItem(QString::number(r.limit_value, 'f', 1)));
        m_tableAlarms->setItem(i, 3, new QTableWidgetItem(QString::number(r.actual_value, 'f', 1)));
        m_tableAlarms->setItem(i, 4, new QTableWidgetItem(
            QDateTime::fromSecsSinceEpoch(r.occur_time).toString("yyyy-MM-dd hh:mm:ss")));
    }
    m_lblAlarmCount->setText(QString("报警总数: %1").arg(m_alarmCounter));
}

void AlarmPage::updateConnectionStatus(bool connected)
{
    if (!connected) {
        m_lblAlarmStatus->setText("● 未连接");
        m_lblAlarmStatus->setStyleSheet("font-size: 16px; font-weight: bold; color: #E74C3C;");
    }
}

void AlarmPage::updateThresholds(double tempMax, double humiMax)
{
    m_tempMaxLimit = tempMax;
    m_humiMaxLimit = humiMax;
    m_spinTempLimit->blockSignals(true);
    m_spinTempLimit->setValue(tempMax);
    m_spinTempLimit->blockSignals(false);
    m_spinHumiLimit->blockSignals(true);
    m_spinHumiLimit->setValue(humiMax);
    m_spinHumiLimit->blockSignals(false);
}

double AlarmPage::tempMaxLimit() const { return m_tempMaxLimit; }
double AlarmPage::humiMaxLimit() const { return m_humiMaxLimit; }
