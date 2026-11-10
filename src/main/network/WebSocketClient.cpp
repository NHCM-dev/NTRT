#include "WebSocketClient.h"
#include <algorithm>

namespace trtnet
{
    WebSocketClient::WebSocketClient(const QUrl& endpoint, const QString& heartbeatRequest, const QString& heartbeatResponse)
        : endpoint(endpoint), heartbeatRequest(heartbeatRequest), heartbeatResponse(heartbeatResponse)
    {
        initSocket();
        initTimers();
    }

    WebSocketClient::~WebSocketClient()
    {
        stop();
    }

    void WebSocketClient::initSocket()
    {
        connect(&socket, &QWebSocket::connected, this, [this]
        {
            activityTimer.start(20000);
            emit connected();
        });
        connect(&socket, &QWebSocket::textMessageReceived, this, [this](const QString& message)
        {
            if (!isConnected())
                return;

            retryDelay = 1000;
            if (!awaitingHeartbeat || message == heartbeatResponse)
            {
                awaitingHeartbeat = false;
                activityTimer.start(20000);
            }
            if (heartbeatResponse.isEmpty() || message != heartbeatResponse)
                emit messageReceived(message);
        });
        connect(&socket, &QWebSocket::disconnected, this, [this]
        {
            reconnect(socket.closeReason().isEmpty() ? QStringLiteral("WebSocket disconnected") : socket.closeReason());
        });
        connect(&socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError)
        {
            reconnect(socket.errorString());
        });
    }

    void WebSocketClient::initTimers()
    {
        retryTimer.setSingleShot(true);
        activityTimer.setSingleShot(true);
        connect(&retryTimer, &QTimer::timeout, this, &WebSocketClient::open);
        connect(&activityTimer, &QTimer::timeout, this, &WebSocketClient::handleTimeout);
    }

    void WebSocketClient::start()
    {
        if (running)
            return;

        running = true;
        retryDelay = 1000;
        open();
    }

    void WebSocketClient::stop()
    {
        running = false;
        retryTimer.stop();
        activityTimer.stop();
        socket.abort();
    }

    void WebSocketClient::open()
    {
        awaitingHeartbeat = false;
        activityTimer.start(15000);
        socket.open(endpoint);
    }

    void WebSocketClient::reconnect(const QString& reason)
    {
        if (!running || retryTimer.isActive())
            return;

        activityTimer.stop();
        retryTimer.start(retryDelay);
        retryDelay = std::min(retryDelay * 2, 30000);
        socket.abort();
        emit connectionFailed(reason);
    }

    void WebSocketClient::handleTimeout()
    {
        if (!isConnected())
            reconnect("WebSocket connection timed out");
        else if (awaitingHeartbeat)
            reconnect("WebSocket heartbeat timed out");
        else if (sendText(heartbeatRequest))
        {
            awaitingHeartbeat = true;
            activityTimer.start(8000);
        }
    }

    bool WebSocketClient::isConnected() const
    {
        return running && !retryTimer.isActive() && socket.state() == QAbstractSocket::ConnectedState;
    }

    bool WebSocketClient::sendText(const QString& message)
    {
        if (!isConnected())
            return false;
        if (socket.sendTextMessage(message) >= 0)
            return true;

        reconnect(socket.errorString());
        return false;
    }
}
