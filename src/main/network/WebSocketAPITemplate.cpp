#include "WebSocketAPITemplate.h"
#include <stdexcept>

namespace trtnet
{
    WebSocketAPITemplate::WebSocketAPITemplate(const QUrl& endpoint, const QString& channel,
                                               const QStringList& requiredParameters)
        : endpoint(endpoint), channel(channel), requiredParameters(requiredParameters)
    {
    }

    const QUrl& WebSocketAPITemplate::getEndpoint() const
    {
        return endpoint;
    }

    QJsonObject WebSocketAPITemplate::createArgument(QJsonObject parameters) const
    {
        for (const QString& key : requiredParameters)
        {
            if (!parameters.contains(key) || parameters.value(key).isNull() || parameters.value(key) == QString())
                throw std::invalid_argument("Missing WebSocket channel parameter: " + key.toStdString());
        }

        parameters.insert("channel", channel);
        return parameters;
    }
}
