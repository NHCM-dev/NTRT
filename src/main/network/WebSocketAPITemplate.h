#ifndef NTRT_WEBSOCKETAPITEMPLATE_H
#define NTRT_WEBSOCKETAPITEMPLATE_H

#include <QJsonObject>
#include <QStringList>
#include <QUrl>

namespace trtnet
{
    class WebSocketAPITemplate final
    {
        QUrl endpoint;
        QString channel;
        QStringList requiredParameters;

    public:
        WebSocketAPITemplate(const QUrl& endpoint, const QString& channel, const QStringList& requiredParameters);
        const QUrl& getEndpoint() const;
        QJsonObject createArgument(QJsonObject parameters) const;
    };
}

#endif //NTRT_WEBSOCKETAPITEMPLATE_H
