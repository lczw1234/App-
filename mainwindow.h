#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QPushButton>
#include <QDialog>
#include <QLabel>
#include "tcpclient.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QString &message);
    void onError(const QString &error);
    void onQueryTimer();

    // 弹窗控制
    void showLedBuzzerDialog();
    void showRelayDialog();

private:
    void appendLog(const QString &text, const QString &color = QStringLiteral("#333333"));
    void updateConnectButton();
    void updateButtonStyle(QPushButton *btn, bool on);
    void sendSetCommand(const QString &dev, int id, int val);
    void sendSetCommand(const QString &dev, int val);
    void sendQuery();
    void parseStatus(const QString &json);
    void setControlsEnabled(bool enabled);

    // 弹窗同步
    void syncDialogButtons();

    void onCmdCooldownEnd();

    Ui::MainWindow *ui;
    TcpClient *m_client;
    QTimer *m_queryTimer;
    QTimer *m_cmdCooldownTimer;
    QTimer *m_statusBlockTimer;
    bool m_isConnected;
    bool m_statusBlocked;
    bool m_cmdCooldown;

    // 设备状态
    bool m_led1On, m_led2On, m_buzzerOn;
    bool m_relay1On, m_relay2On, m_relay3On, m_relay4On;

    // 连接状态透明消息覆盖层
    QLabel *m_connectOverlay;

    // 弹窗指针
    QDialog *m_ledBuzzerDlg;
    QDialog *m_relayDlg;
    QPushButton *m_dlgLed1Btn, *m_dlgLed2Btn, *m_dlgBuzzerBtn;
    QPushButton *m_dlgRelay1Btn, *m_dlgRelay2Btn, *m_dlgRelay3Btn, *m_dlgRelay4Btn;
};

#endif // MAINWINDOW_H
