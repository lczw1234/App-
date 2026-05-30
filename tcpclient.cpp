#include "tcpclient.h"
#include <QDebug>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_reconnectTimer(new QTimer(this))
    , m_connectTimer(new QTimer(this))
    , m_port(0)
    , m_autoReconnect(true)
{
    connect(m_socket, &QTcpSocket::connected,
            this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TcpClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &TcpClient::onError);

    m_reconnectTimer->setInterval(3000);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &TcpClient::onReconnectTimer);

    // 连接超时：10 秒
    m_connectTimer->setSingleShot(true);
    connect(m_connectTimer, &QTimer::timeout,
            this, &TcpClient::onConnectTimeout);
}

TcpClient::~TcpClient()
{
    disconnectFromServer();
}

void TcpClient::connectToServer(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
    m_autoReconnect = true;

    m_socket->connectToHost(host, port);
    m_connectTimer->start(10000);  // 10 秒超时
}

void TcpClient::disconnectFromServer()
{
    m_autoReconnect = false;
    m_reconnectTimer->stop();
    m_connectTimer->stop();
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

void TcpClient::sendMessage(const QString &message)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray data = message.toUtf8() + "\n";
        m_socket->write(data);
        m_socket->flush();
    } else {
        emit errorOccurred(QStringLiteral("未连接到服务器，无法发送消息"));
    }
}

bool TcpClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpClient::onConnected()
{
    m_connectTimer->stop();
    m_reconnectTimer->stop();
    emit connected();
    emit connectionStateChanged(true);
}

void TcpClient::onDisconnected()
{
    m_connectTimer->stop();
    emit disconnected();
    emit connectionStateChanged(false);

    if (m_autoReconnect && !m_host.isEmpty()) {
        m_reconnectTimer->start();
    }
}

void TcpClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QString message = QString::fromUtf8(data);
    emit messageReceived(message);
}

void TcpClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    m_connectTimer->stop();
    QString errorMsg = m_socket->errorString();

    // 提供更友好的中文提示
    if (errorMsg.contains(QStringLiteral("timeout"), Qt::CaseInsensitive) ||
        errorMsg.contains(QStringLiteral("timed out"), Qt::CaseInsensitive)) {
        errorMsg = QStringLiteral("连接超时。请确保手机已连接 ESP8266 的 WiFi，且设备已上电运行。");
    } else if (errorMsg.contains(QStringLiteral("refused"), Qt::CaseInsensitive)) {
        errorMsg = QStringLiteral("连接被拒绝。请检查 ESP8266 端口 333 是否已开启。");
    } else if (errorMsg.contains(QStringLiteral("network"), Qt::CaseInsensitive) ||
               errorMsg.contains(QStringLiteral("unreachable"), Qt::CaseInsensitive)) {
        errorMsg = QStringLiteral("网络不可达。请确认手机已连接 ESP8266 的 WiFi 热点。");
    }

    emit errorOccurred(errorMsg);

    if (m_autoReconnect && !m_host.isEmpty()) {
        m_reconnectTimer->start();
    }
}

void TcpClient::onReconnectTimer()
{
    if (!m_host.isEmpty() && m_port > 0) {
        m_socket->connectToHost(m_host, m_port);
        m_connectTimer->start(10000);
    }
}

void TcpClient::onConnectTimeout()
{
    // 主动断开并报告超时
    m_socket->abort();
    emit errorOccurred(QStringLiteral("连接超时（10秒）。请确保：\n"
                                       "1. 手机已连接 ESP8266 的 WiFi\n"
                                       "2. ESP8266/STM32 设备已上电\n"
                                       "3. IP 地址 192.168.4.1 端口 333 正确"));
}
