#ifndef NTRT_WEBSOCKETCLIENT_H
#define NTRT_WEBSOCKETCLIENT_H

#include <QTimer>
#include <QWebSocket>

namespace trtnet
{
    class WebSocketClient final : public QObject
    {
        Q_OBJECT

        QWebSocket socket;
        QTimer retryTimer;
        QTimer activityTimer;
        QUrl endpoint;
        QString heartbeatRequest;
        QString heartbeatResponse;
        int retryDelay = 1000;
        bool running = false;
        bool awaitingHeartbeat = false;

        void initSocket();
        void initTimers();
        void open();
        void handleTimeout();

    public:
        WebSocketClient(const QUrl& endpoint, const QString& heartbeatRequest, const QString& heartbeatResponse);
        ~WebSocketClient() override;
        void start();
        void stop();
        void reconnect(const QString& reason);
        bool isConnected() const;
        bool sendText(const QString& message);

    signals:
        void connected();
        void messageReceived(const QString& message);
        void connectionFailed(const QString& reason);
    };
}

#endif //NTRT_WEBSOCKETCLIENT_H
