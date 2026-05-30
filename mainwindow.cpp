#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_client(new TcpClient(this))
    , m_queryTimer(new QTimer(this))
    , m_isConnected(false)
    , m_statusBlocked(false)
    , m_led1On(false), m_led2On(false), m_buzzerOn(false)
    , m_relay1On(false), m_relay2On(false), m_relay3On(false), m_relay4On(false)
    , m_ledBuzzerDlg(nullptr), m_relayDlg(nullptr)
    , m_dlgLed1Btn(nullptr), m_dlgLed2Btn(nullptr), m_dlgBuzzerBtn(nullptr)
    , m_dlgRelay1Btn(nullptr), m_dlgRelay2Btn(nullptr), m_dlgRelay3Btn(nullptr), m_dlgRelay4Btn(nullptr)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("智能家居"));
    resize(440, 760);

    ui->ipLineEdit->setText(QStringLiteral("192.168.4.1"));
    ui->portLineEdit->setText(QStringLiteral("333"));

    // ==================== 登录界面样式 ====================
    // 整体背景
    ui->scrollContent->setStyleSheet(QStringLiteral(
        "QWidget#scrollContent { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #E8F5E9, stop:1 #C8E6C9); }"));

    // 登录卡片
    ui->connectionGroupBox->setStyleSheet(QStringLiteral(
        "QGroupBox {"
        "  background: white; border-radius: 24px;"
        "  border: 2px solid #A5D6A7;"
        "}"));

    // 标题
    ui->loginTitle->setStyleSheet(QStringLiteral(
        "font-size:24pt; font-weight:bold; color:#1B5E20; padding:8px 0;"));

    // 登录图标
    QPixmap iconPixmap(QStringLiteral(":/icon.png"));
    if (!iconPixmap.isNull()) {
        ui->loginIcon->setPixmap(iconPixmap.scaledToHeight(256, Qt::SmoothTransformation));
    }

    // 输入框标签
    ui->ipLabel->setStyleSheet(QStringLiteral(
        "font-size:24pt; font-weight:bold; color:#33691E;"));
    ui->portLabel->setStyleSheet(QStringLiteral(
        "font-size:24pt; font-weight:bold; color:#33691E;"));

    // 输入框
    QString inputStyle = QStringLiteral(
        "QLineEdit {"
        "  font-size:24pt; padding:16px 20px;"
        "  border: 2px solid #C8E6C9; border-radius: 14px;"
        "  background: #FAFAFA; color: #333;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #66BB6A; background: white;"
        "}");
    ui->ipLineEdit->setStyleSheet(inputStyle);
    ui->portLineEdit->setStyleSheet(inputStyle);

    // 连接状态透明消息覆盖层
    m_connectOverlay = new QLabel(ui->loginPage);
    m_connectOverlay->setAlignment(Qt::AlignCenter);
    m_connectOverlay->setWordWrap(true);
    m_connectOverlay->setStyleSheet(QStringLiteral(
        "QLabel { background: transparent; color: black;"
        "  font-size: 16pt; font-weight: bold; border-radius: 16px; padding: 24px; }"));
    m_connectOverlay->hide();

    // 状态
    ui->statusLabel->setStyleSheet(QStringLiteral(
        "color:#E53935; font-weight:bold; font-size:20pt; padding:6px 0;"));

    // 连接按钮
    connect(ui->connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);

    // 设备控制入口按钮
    connect(ui->ledBuzzerBtn, &QPushButton::clicked,
            this, &MainWindow::showLedBuzzerDialog);
    connect(ui->relayBtn, &QPushButton::clicked,
            this, &MainWindow::showRelayDialog);
    ui->ledBuzzerBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #66BB6A, stop:1 #43A047); color: white; border: none;"
        "  border-radius: 44px; padding: 46px; font-size: 20pt; font-weight: bold; }"
        "QPushButton:pressed { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #43A047, stop:1 #2E7D32); }"));
    ui->relayBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #66BB6A, stop:1 #43A047); color: white; border: none;"
        "  border-radius: 44px; padding: 46px; font-size: 20pt; font-weight: bold; }"
        "QPushButton:pressed { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #43A047, stop:1 #2E7D32); }"));

    // TCP 信号
    connect(m_client, &TcpClient::connected,
            this, &MainWindow::onConnected);
    connect(m_client, &TcpClient::disconnected,
            this, &MainWindow::onDisconnected);
    connect(m_client, &TcpClient::messageReceived,
            this, &MainWindow::onMessageReceived);
    connect(m_client, &TcpClient::errorOccurred,
            this, &MainWindow::onError);

    // 查询定时器
    m_queryTimer->setInterval(5000);
    connect(m_queryTimer, &QTimer::timeout,
            this, &MainWindow::onQueryTimer);

    // 命令冷却定时器（防止按钮连点）
    m_cmdCooldownTimer = new QTimer(this);
    m_cmdCooldownTimer->setSingleShot(true);
    m_cmdCooldownTimer->setInterval(300);
    connect(m_cmdCooldownTimer, &QTimer::timeout,
            this, &MainWindow::onCmdCooldownEnd);
    m_cmdCooldown = false;

    // 状态封锁定时器：发命令后1.5秒内不接收设备状态更新，防止旧查询回复覆盖本地状态
    m_statusBlockTimer = new QTimer(this);
    m_statusBlockTimer->setSingleShot(true);
    m_statusBlockTimer->setInterval(1500);
    connect(m_statusBlockTimer, &QTimer::timeout, this, [this]() {
        m_statusBlocked = false;
    });

    // 初始禁用控制按钮
    ui->ledBuzzerBtn->setEnabled(false);
    ui->relayBtn->setEnabled(false);

    // 主页面设备按钮最小高度
    ui->ledBuzzerBtn->setMinimumHeight(100);
    ui->relayBtn->setMinimumHeight(100);

    // 传感器字体放大
    ui->gasLabel->setStyleSheet(QStringLiteral("font-size:20pt;font-weight:bold;color:#3E543E;"));
    ui->lightLabel->setStyleSheet(QStringLiteral("font-size:20pt;font-weight:bold;color:#3E543E;"));
    ui->gasValue->setStyleSheet(QStringLiteral("font-weight:bold;font-size:20pt;color:#2E7D32;"));
    ui->lightValue->setStyleSheet(QStringLiteral("font-weight:bold;font-size:20pt;color:#2E7D32;"));
    ui->tempLabel->setStyleSheet(QStringLiteral("font-size:20pt;color:#3E543E;background:#F1F8E9;border-radius:12px;padding:14px 14px;"));
    ui->humiLabel->setStyleSheet(QStringLiteral("font-size:20pt;color:#3E543E;background:#F1F8E9;border-radius:12px;padding:14px 14px;"));
    ui->co2Label->setStyleSheet(QStringLiteral("font-size:20pt;color:#3E543E;background:#F1F8E9;border-radius:12px;padding:14px 14px;"));

    // 日志框字体
    ui->logTextEdit->setStyleSheet(QStringLiteral("font-size:20pt;"));

    // ==================== 主页面 50/50 上下分割 ====================
    // 上半容器：设备控制 + 传感器数据
    QWidget *topPanel = new QWidget;
    QVBoxLayout *topPanelLayout = new QVBoxLayout(topPanel);
    topPanelLayout->setContentsMargins(0, 0, 0, 0);
    topPanelLayout->setSpacing(14);

    ui->mainPageLayout->removeWidget(ui->controlGroupBox);
    ui->mainPageLayout->removeWidget(ui->sensorGroupBox);
    topPanelLayout->addWidget(ui->controlGroupBox);
    topPanelLayout->addWidget(ui->sensorGroupBox);

    ui->mainPageLayout->insertWidget(0, topPanel);

    // 上下各占 50%
    ui->mainPageLayout->setStretchFactor(topPanel, 1);
    ui->mainPageLayout->setStretchFactor(ui->logGroupBox, 1);

    // 初始显示登录页面
    ui->stackedWidget->setCurrentWidget(ui->loginPage);

    updateConnectButton();
}

MainWindow::~MainWindow()
{
    m_queryTimer->stop();
    m_client->disconnectFromServer();
    if (m_ledBuzzerDlg) { m_ledBuzzerDlg->close(); delete m_ledBuzzerDlg; }
    if (m_relayDlg) { m_relayDlg->close(); delete m_relayDlg; }
    delete ui;
}

// ==================== 弹窗：LED & 蜂鸣器 ====================

void MainWindow::showLedBuzzerDialog()
{
    if (!m_isConnected) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先连接服务器"));
        return;
    }

    if (m_ledBuzzerDlg) {
        m_ledBuzzerDlg->close();
        delete m_ledBuzzerDlg;
    }

    QDialog *dlg = new QDialog(this);
    m_ledBuzzerDlg = dlg;
    dlg->setWindowTitle(QStringLiteral("LED 灯 & 蜂鸣器"));
    dlg->setMinimumWidth(800);
    dlg->setStyleSheet(QStringLiteral(
        "QDialog { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #E8F5E9, stop:1 #F1F8E9); }"));

    QVBoxLayout *layout = new QVBoxLayout(dlg);
    layout->setSpacing(24);
    layout->setContentsMargins(34, 34, 34, 34);

    // 标题
    QLabel *title = new QLabel(QStringLiteral("LED 灯 & 蜂鸣器控制"), dlg);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "font-size:20pt; font-weight:bold; color:#1B5E20; padding:8px 0;"));
    layout->addWidget(title);

    // LED 1
    QPushButton *led1 = new QPushButton(QStringLiteral("LED 灯 1"), dlg);
    led1->setCheckable(true);
    led1->setChecked(m_led1On);
    led1->setMinimumHeight(100);
    updateButtonStyle(led1, m_led1On);
    connect(led1, &QPushButton::toggled, this, [this](bool on) {
        sendSetCommand(QStringLiteral("led"), 1, on ? 1 : 0);
        m_led1On = on;
        updateButtonStyle(m_dlgLed1Btn, on);
        appendLog(QStringLiteral("LED 1 → %1").arg(on ? QStringLiteral("开") : QStringLiteral("关")));
    });
    layout->addWidget(led1);
    m_dlgLed1Btn = led1;

    // LED 2
    QPushButton *led2 = new QPushButton(QStringLiteral("LED 灯 2"), dlg);
    led2->setCheckable(true);
    led2->setChecked(m_led2On);
    led2->setMinimumHeight(100);
    updateButtonStyle(led2, m_led2On);
    connect(led2, &QPushButton::toggled, this, [this](bool on) {
        sendSetCommand(QStringLiteral("led"), 2, on ? 1 : 0);
        m_led2On = on;
        updateButtonStyle(m_dlgLed2Btn, on);
        appendLog(QStringLiteral("LED 2 → %1").arg(on ? QStringLiteral("开") : QStringLiteral("关")));
    });
    layout->addWidget(led2);
    m_dlgLed2Btn = led2;

    // Buzzer
    QPushButton *buzzer = new QPushButton(QStringLiteral("蜂  鸣  器"), dlg);
    buzzer->setCheckable(true);
    buzzer->setChecked(m_buzzerOn);
    buzzer->setMinimumHeight(100);
    updateButtonStyle(buzzer, m_buzzerOn);
    connect(buzzer, &QPushButton::toggled, this, [this](bool on) {
        sendSetCommand(QStringLiteral("buzzer"), on ? 1 : 0);
        m_buzzerOn = on;
        updateButtonStyle(m_dlgBuzzerBtn, on);
        appendLog(QStringLiteral("蜂鸣器 → %1").arg(on ? QStringLiteral("开") : QStringLiteral("关")));
    });
    layout->addWidget(buzzer);
    m_dlgBuzzerBtn = buzzer;

    // 关闭按钮
    QPushButton *closeBtn = new QPushButton(QStringLiteral("关  闭"), dlg);
    closeBtn->setMinimumHeight(100);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color:#9E9E9E; color:white; border:none;"
        "  border-radius:34px; font-size:20pt; font-weight:bold; padding:34px; }"
        "QPushButton:pressed { background-color:#757575; }"));
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    layout->addWidget(closeBtn);

    connect(dlg, &QDialog::finished, this, [this]() {
        m_ledBuzzerDlg = nullptr;
        m_dlgLed1Btn = nullptr;
        m_dlgLed2Btn = nullptr;
        m_dlgBuzzerBtn = nullptr;
    });

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

// ==================== 弹窗：继电器 ====================

void MainWindow::showRelayDialog()
{
    if (!m_isConnected) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先连接服务器"));
        return;
    }

    if (m_relayDlg) {
        m_relayDlg->close();
        delete m_relayDlg;
    }

    QDialog *dlg = new QDialog(this);
    m_relayDlg = dlg;
    dlg->setWindowTitle(QStringLiteral("继电器控制"));
    dlg->setMinimumWidth(800);
    dlg->setStyleSheet(QStringLiteral(
        "QDialog { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #E8F5E9, stop:1 #F1F8E9); }"));

    QVBoxLayout *layout = new QVBoxLayout(dlg);
    layout->setSpacing(24);
    layout->setContentsMargins(34, 34, 34, 34);

    QLabel *title = new QLabel(QStringLiteral("继电器控制"), dlg);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "font-size:20pt; font-weight:bold; color:#1B5E20; padding:8px 0;"));
    layout->addWidget(title);

    struct RelayInfo {
        QString name;
        int id;
        bool state;
        QPushButton **btnPtr;
    };

    RelayInfo relays[] = {
        {QStringLiteral("风  扇  (继电器 1)"), 1, m_relay1On, &m_dlgRelay1Btn},
        {QStringLiteral("空  调  (继电器 2)"), 2, m_relay2On, &m_dlgRelay2Btn},
        {QStringLiteral("水  泵  (继电器 3)"), 3, m_relay3On, &m_dlgRelay3Btn},
        {QStringLiteral("窗  户  (继电器 4)"), 4, m_relay4On, &m_dlgRelay4Btn},
    };

    for (auto &r : relays) {
        QPushButton *btn = new QPushButton(r.name, dlg);
        btn->setCheckable(true);
        btn->setChecked(r.state);
        btn->setMinimumHeight(100);
        updateButtonStyle(btn, r.state);

        int relayId = r.id;
        connect(btn, &QPushButton::toggled, this, [this, relayId](bool on) {
            sendSetCommand(QStringLiteral("relay"), relayId, on ? 1 : 0);
            switch (relayId) {
            case 1: m_relay1On = on; break;
            case 2: m_relay2On = on; break;
            case 3: m_relay3On = on; break;
            case 4: m_relay4On = on; break;
            }
            syncDialogButtons();
            QStringList names = {QStringLiteral("风扇"), QStringLiteral("空调"),
                                 QStringLiteral("水泵"), QStringLiteral("窗户")};
            appendLog(QStringLiteral("%1 → %2").arg(names[relayId-1],
                      on ? QStringLiteral("开") : QStringLiteral("关")));
        });

        layout->addWidget(btn);
        *r.btnPtr = btn;
    }

    // 关闭
    QPushButton *closeBtn = new QPushButton(QStringLiteral("关  闭"), dlg);
    closeBtn->setMinimumHeight(100);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color:#9E9E9E; color:white; border:none;"
        "  border-radius:34px; font-size:20pt; font-weight:bold; padding:34px; }"
        "QPushButton:pressed { background-color:#757575; }"));
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    layout->addWidget(closeBtn);

    connect(dlg, &QDialog::finished, this, [this]() {
        m_relayDlg = nullptr;
        m_dlgRelay1Btn = nullptr;
        m_dlgRelay2Btn = nullptr;
        m_dlgRelay3Btn = nullptr;
        m_dlgRelay4Btn = nullptr;
    });

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

// ==================== 连接管理 ====================

void MainWindow::onConnectClicked()
{
    if (m_isConnected) {
        m_queryTimer->stop();
        if (m_ledBuzzerDlg) m_ledBuzzerDlg->close();
        if (m_relayDlg) m_relayDlg->close();
        m_client->disconnectFromServer();
    } else {
        QString host = ui->ipLineEdit->text().trimmed();
        quint16 port = static_cast<quint16>(ui->portLineEdit->text().toUInt());

        if (host.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"),
                                 QStringLiteral("请输入 ESP8266 的 IP 地址"));
            return;
        }

        ui->statusLabel->setText(QStringLiteral("● 正在连接..."));
        ui->statusLabel->setStyleSheet(QStringLiteral("color:#F57C00;font-weight:bold;font-size:20pt; padding:6px 0;"));
        ui->connectButton->setEnabled(false);
        ui->connectButton->setText(QStringLiteral("连接中..."));

        // 登录界面弹出透明消息框
        QString overlayMsg = QStringLiteral("提示：请确保手机已连接 ESP8266 的 WiFi 热点");
        m_connectOverlay->setText(overlayMsg);
        m_connectOverlay->setGeometry(20, 400, ui->loginPage->width() - 40, 160);
        m_connectOverlay->raise();
        m_connectOverlay->show();

        m_client->connectToServer(host, port);
    }
}

void MainWindow::onConnected()
{
    m_connectOverlay->hide();
    m_isConnected = true;
    updateConnectButton();
    ui->ledBuzzerBtn->setEnabled(true);
    ui->relayBtn->setEnabled(true);
    ui->stackedWidget->setCurrentWidget(ui->mainPage);
    ui->statusLabel->setText(QStringLiteral("● 已连接"));
    ui->statusLabel->setStyleSheet(QStringLiteral("color:#2E7D32;font-weight:bold;font-size:20pt; padding:6px 0;"));
    appendLog(QStringLiteral("已连接到 ESP8266 服务器"), QStringLiteral("#2E7D32"));

    // 延迟500ms再查询，等ESP8266串口桥初始化完成
    QTimer::singleShot(500, this, [this]() {
        if (m_isConnected) {
            sendQuery();
            m_queryTimer->start();
        }
    });
}

void MainWindow::onDisconnected()
{
    m_isConnected = false;
    m_queryTimer->stop();
    updateConnectButton();
    ui->ledBuzzerBtn->setEnabled(false);
    ui->relayBtn->setEnabled(false);
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
    if (m_ledBuzzerDlg) m_ledBuzzerDlg->close();
    if (m_relayDlg) m_relayDlg->close();
    ui->statusLabel->setText(QStringLiteral("● 未连接"));
    ui->statusLabel->setStyleSheet(QStringLiteral("color:#E53935;font-weight:bold;font-size:20pt; padding:6px 0;"));
    appendLog(QStringLiteral("连接已断开"), QStringLiteral("#E53935"));
}

// ==================== 消息处理 ====================

void MainWindow::onMessageReceived(const QString &message)
{
    appendLog(QStringLiteral("收到: ") + message, QStringLiteral("#336699"));
    parseStatus(message);
}

void MainWindow::onError(const QString &error)
{
    if (!m_isConnected) {
        m_connectOverlay->hide();
        ui->connectButton->setEnabled(true);
        ui->connectButton->setText(QStringLiteral("连 接 服 务 器"));
        ui->statusLabel->setText(QStringLiteral("● 未连接"));
        ui->statusLabel->setStyleSheet(QStringLiteral("color:#E53935;font-weight:bold;font-size:20pt; padding:6px 0;"));
    }
    appendLog(QStringLiteral("错误: ") + error, QStringLiteral("#CC0000"));
}

void MainWindow::onQueryTimer()
{
    if (m_isConnected) sendQuery();
}

// ==================== 命令发送 ====================

void MainWindow::sendSetCommand(const QString &dev, int id, int val)
{
    if (!m_isConnected || m_cmdCooldown) return;
    m_cmdCooldown = true;
    m_cmdCooldownTimer->start();
    m_statusBlocked = true;
    m_statusBlockTimer->start();

    QJsonObject cmd;
    cmd[QStringLiteral("cmd")] = QStringLiteral("set");
    cmd[QStringLiteral("dev")] = dev;
    cmd[QStringLiteral("id")] = id;
    cmd[QStringLiteral("val")] = val;
    QJsonDocument doc(cmd);
    m_client->sendMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    // 重启查询定时器，给STM32足够时间处理命令后再查询状态
    m_queryTimer->start();
}

void MainWindow::sendSetCommand(const QString &dev, int val)
{
    if (!m_isConnected || m_cmdCooldown) return;
    m_cmdCooldown = true;
    m_cmdCooldownTimer->start();
    m_statusBlocked = true;
    m_statusBlockTimer->start();

    QJsonObject cmd;
    cmd[QStringLiteral("cmd")] = QStringLiteral("set");
    cmd[QStringLiteral("dev")] = dev;
    cmd[QStringLiteral("val")] = val;
    QJsonDocument doc(cmd);
    m_client->sendMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    // 重启查询定时器，给STM32足够时间处理命令后再查询状态
    m_queryTimer->start();
}

void MainWindow::onCmdCooldownEnd()
{
    m_cmdCooldown = false;
}

void MainWindow::sendQuery()
{
    if (!m_isConnected) return;
    QJsonObject cmd;
    cmd[QStringLiteral("cmd")] = QStringLiteral("query");
    QJsonDocument doc(cmd);
    m_client->sendMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

// ==================== 状态解析 ====================

void MainWindow::parseStatus(const QString &json)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return;

    QJsonObject obj = doc.object();
    if (obj.value(QStringLiteral("type")).toString() != QStringLiteral("status")) return;

    // 状态封锁期间跳过设备状态更新，防止旧查询回复覆盖刚操作的状态
    if (!m_statusBlocked) {
        // LED
        QJsonArray led = obj.value(QStringLiteral("led")).toArray();
        if (led.size() >= 2) {
            m_led1On = (led.at(0).toInt() == 1);
            m_led2On = (led.at(1).toInt() == 1);
        }

        // Relay
        QJsonArray relay = obj.value(QStringLiteral("relay")).toArray();
        if (relay.size() >= 4) {
            m_relay1On = (relay.at(0).toInt() == 1);
            m_relay2On = (relay.at(1).toInt() == 1);
            m_relay3On = (relay.at(2).toInt() == 1);
            m_relay4On = (relay.at(3).toInt() == 1);
        }

        // Buzzer
        m_buzzerOn = (obj.value(QStringLiteral("buzzer")).toInt() == 1);

        // 同步弹窗按钮
        syncDialogButtons();
    }

    // 传感器（始终更新，不受状态封锁影响）
    int gas = obj.value(QStringLiteral("gas")).toInt();
    int light = obj.value(QStringLiteral("light")).toInt();
    int temp = obj.value(QStringLiteral("temp")).toInt();
    int humi = obj.value(QStringLiteral("humi")).toInt();
    int co2 = obj.value(QStringLiteral("co2")).toInt();

    ui->gasBar->setValue(gas);
    ui->gasValue->setText(QString::number(gas));
    ui->lightBar->setValue(light);
    ui->lightValue->setText(QString::number(light));
    ui->tempLabel->setText(QStringLiteral("温度  %1 °C").arg(temp));
    ui->humiLabel->setText(QStringLiteral("湿度  %1 %").arg(humi));
    ui->co2Label->setText(QStringLiteral("CO2   %1").arg(co2));
}

void MainWindow::syncDialogButtons()
{
    // LED & Buzzer 弹窗
    if (m_dlgLed1Btn) {
        QSignalBlocker blocker(m_dlgLed1Btn);
        m_dlgLed1Btn->setChecked(m_led1On);
        updateButtonStyle(m_dlgLed1Btn, m_led1On);
    }
    if (m_dlgLed2Btn) {
        QSignalBlocker blocker(m_dlgLed2Btn);
        m_dlgLed2Btn->setChecked(m_led2On);
        updateButtonStyle(m_dlgLed2Btn, m_led2On);
    }
    if (m_dlgBuzzerBtn) {
        QSignalBlocker blocker(m_dlgBuzzerBtn);
        m_dlgBuzzerBtn->setChecked(m_buzzerOn);
        updateButtonStyle(m_dlgBuzzerBtn, m_buzzerOn);
    }

    // 继电器弹窗
    if (m_dlgRelay1Btn) {
        QSignalBlocker blocker(m_dlgRelay1Btn);
        m_dlgRelay1Btn->setChecked(m_relay1On);
        updateButtonStyle(m_dlgRelay1Btn, m_relay1On);
    }
    if (m_dlgRelay2Btn) {
        QSignalBlocker blocker(m_dlgRelay2Btn);
        m_dlgRelay2Btn->setChecked(m_relay2On);
        updateButtonStyle(m_dlgRelay2Btn, m_relay2On);
    }
    if (m_dlgRelay3Btn) {
        QSignalBlocker blocker(m_dlgRelay3Btn);
        m_dlgRelay3Btn->setChecked(m_relay3On);
        updateButtonStyle(m_dlgRelay3Btn, m_relay3On);
    }
    if (m_dlgRelay4Btn) {
        QSignalBlocker blocker(m_dlgRelay4Btn);
        m_dlgRelay4Btn->setChecked(m_relay4On);
        updateButtonStyle(m_dlgRelay4Btn, m_relay4On);
    }
}

// ==================== UI 辅助 ====================

void MainWindow::updateButtonStyle(QPushButton *btn, bool on)
{
    if (on) {
        btn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "    stop:0 #66BB6A, stop:1 #43A047);"
            "  color: white;"
            "  border: none; border-radius: 20px;"
            "  padding: 36px 46px; font-size: 20pt; font-weight: bold;"
            "}"
            "QPushButton:pressed {"
            "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "    stop:0 #43A047, stop:1 #2E7D32);"
            "}"
        ));
    } else {
        btn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #E8EDE8; color: #6A7A6A;"
            "  border: 22px solid #D0D8D0; border-radius: 34px;"
            "  padding: 36px 36px; font-size: 20pt;"
            "}"
            "QPushButton:pressed { background-color: #D0D8D0; }"
        ));
    }
}

void MainWindow::setControlsEnabled(bool enabled)
{
    ui->ledBuzzerBtn->setEnabled(enabled);
    ui->relayBtn->setEnabled(enabled);
}

void MainWindow::updateConnectButton()
{
    ui->connectButton->setEnabled(true);
    ui->connectButton->setMinimumHeight(100);
    if (m_isConnected) {
        ui->connectButton->setText(QStringLiteral("断开连接"));
        ui->connectButton->setStyleSheet(
            QStringLiteral("QPushButton {"
                           "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                           "    stop:0 #EF5350, stop:1 #C62828);"
                           "  color: white;"
                           "  border: none; border-radius: 44px;"
                           "  padding: 44px; font-size: 20pt; font-weight: bold;"
                           "}"
                           "QPushButton:pressed {"
                           "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                           "    stop:0 #C62828, stop:1 #B71C1C);"
                           "}"));
    } else {
        ui->connectButton->setText(QStringLiteral("连接服务器"));
        ui->connectButton->setStyleSheet(
            QStringLiteral("QPushButton {"
                           "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                           "    stop:0 #66BB6A, stop:1 #388E3C);"
                           "  color: white;"
                           "  border: none; border-radius: 44px;"
                           "  padding: 44px; font-size: 20pt; font-weight: bold;"
                           "}"
                           "QPushButton:pressed {"
                           "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                           "    stop:0 #388E3C, stop:1 #1B5E20);"
                           "}"));
    }
}

void MainWindow::appendLog(const QString &text, const QString &color)
{
    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    ui->logTextEdit->append(
        QStringLiteral("<span style='color:%1;'>[%2] %3</span>")
            .arg(color, timestamp, text.toHtmlEscaped()));
}
