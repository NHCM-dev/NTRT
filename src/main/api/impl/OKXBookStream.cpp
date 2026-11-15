#include "OKXBookStream.h"
#include "main/network/WebSocketAPITemplate.h"
#include "main/util/OKXBookUtil.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace trtdata::okx
{
    static const trtnet::WebSocketAPITemplate BOOKS_CHANNEL{
        QUrl{"wss://ws.okx.com/ws/v5/public"}, "books", {"instId"}
    };

    OKXBookStream::OKXBookStream() : client(BOOKS_CHANNEL.getEndpoint(), "ping", "pong")
    {
        connect(&client, &trtnet::WebSocketClient::connected, this, [this]
        {
            invalidate("Waiting for book snapshot…");
            std::set<std::string> sent;
            for (const InstIdentifier& instrument : instruments)
            {
                if (sent.insert(instrument.id).second)
                    sendSubscription("subscribe", instrument);
            }
        });
        connect(&client, &trtnet::WebSocketClient::connectionFailed, this, &OKXBookStream::invalidate);
        connect(&client, &trtnet::WebSocketClient::messageReceived, this, &OKXBookStream::handleMessage);
    }

    void OKXBookStream::invalidate(const QString& reason)
    {
        books.clear();
        for (const InstIdentifier& instrument : instruments)
            emit bookReceived(instrument, {{}, {}, 0, -1, reason});
    }

    void OKXBookStream::sendSubscription(const QString& operation, const InstIdentifier& instrument)
    {
        const QJsonArray arguments{BOOKS_CHANNEL.createArgument({{"instId", QString::fromStdString(instrument.id)}})};
        const QJsonDocument request(QJsonObject{{"op", operation}, {"args", arguments}});
        client.sendText(QString::fromUtf8(request.toJson(QJsonDocument::Compact)));
    }

    void OKXBookStream::setInstruments(const Instruments& desired)
    {
        if (desired == instruments)
            return;
        const Instruments previous = instruments;
        instruments = desired;
        std::set<std::string> removed;
        for (const InstIdentifier& instrument : previous)
        {
            if (!desired.contains(instrument))
            {
                books.erase(instrument);
                const bool retained = std::any_of(desired.begin(), desired.end(), [&instrument](const InstIdentifier& value)
                {
                    return value.id == instrument.id;
                });
                if (client.isConnected() && !retained && removed.insert(instrument.id).second)
                    sendSubscription("unsubscribe", instrument);
            }
        }
        if (desired.empty())
            stop();
        else if (!client.isConnected())
            client.start();
        else
        {
            std::set<std::string> added;
            for (const InstIdentifier& instrument : desired)
            {
                const bool subscribed = std::any_of(previous.begin(), previous.end(), [&instrument](const InstIdentifier& value)
                {
                    return value.id == instrument.id;
                });
                if (!subscribed && added.insert(instrument.id).second)
                    sendSubscription("subscribe", instrument);
            }
        }
    }

    void OKXBookStream::stop()
    {
        client.stop();
        instruments.clear();
        books.clear();
    }

    void OKXBookStream::handleMessage(const QString& message)
    {
        try
        {
            const jsonElement response = parseElement(message.toUtf8()).value();
            std::string_view event;
            if (response["event"].get_string().get(event) == simdjson::SUCCESS)
            {
                if (event == "notice")
                    client.reconnect(QString::fromUtf8(readString(response, "msg")));
                else if (event == "error")
                    invalidate(QString::fromUtf8(readString(response, "msg")));
                return;
            }
            const jsonElement argument = response["arg"].value();
            if (readString(argument, "channel") != "books")
                return;
            const std::string_view id = readString(argument, "instId");
            const std::string_view action = readString(response, "action");
            if (action != "snapshot" && action != "update")
                return;
            for (const InstIdentifier& instrument : instruments)
            {
                if (instrument.id != id)
                    continue;
                for (const jsonElement row : readArray(response, "data"))
                {
                    trttrade::BookSnapshot& book = books[instrument];
                    if (!applyBookMessage(row, action == "snapshot", book))
                    {
                        client.reconnect("Order book sequence gap · Resynchronizing");
                        return;
                    }
                    emit bookReceived(instrument, book);
                }
            }
        } catch (const simdjson::simdjson_error&)
        {
            client.reconnect("Malformed order book · Resynchronizing");
        } catch (const std::runtime_error&)
        {
            client.reconnect("Invalid order book level · Resynchronizing");
        }
    }
}
