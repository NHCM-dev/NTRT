#include "OKXTickerStream.h"
#include "main/data/parser/OKXParser.h"
#include "main/network/WebSocketAPITemplate.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace trtdata::okx
{
    static const trtnet::WebSocketAPITemplate TICKERS_CHANNEL{QUrl{"wss://ws.okx.com/ws/v5/public"}, "tickers", {"instId"}};

    OKXTickerStream::OKXTickerStream()
        : client(TICKERS_CHANNEL.getEndpoint(), "ping", "pong")
    {
        initClient();
    }

    void OKXTickerStream::initClient()
    {
        connect(&client, &trtnet::WebSocketClient::connected, this, [this]
        {
            emit connectionChanged({});
            syncSubscriptions({});
        });
        connect(&client, &trtnet::WebSocketClient::connectionFailed, this, [this](const QString& reason)
        {
            pendingSubscriptions.clear();
            emit connectionChanged(reason);
        });
        connect(&client, &trtnet::WebSocketClient::messageReceived, this, &OKXTickerStream::handleMessage);
    }

    void OKXTickerStream::setInstruments(const Instruments& desired)
    {
        if (desired == instruments)
            return;

        Instruments previous = std::exchange(instruments, desired);
        if (instruments.empty())
            stop();
        else if (client.isConnected())
            syncSubscriptions(previous);
        else
            client.start();
    }

    void OKXTickerStream::stop()
    {
        client.stop();
        instruments.clear();
        pendingSubscriptions.clear();
    }

    void OKXTickerStream::syncSubscriptions(const Instruments& previous)
    {
        Instruments removed;
        Instruments added;
        std::set_difference(previous.begin(), previous.end(), instruments.begin(), instruments.end(),
                            std::inserter(removed, removed.end()));
        std::set_difference(instruments.begin(), instruments.end(), previous.begin(), previous.end(),
                            std::inserter(added, added.end()));
        if (sendSubscriptions("unsubscribe", removed))
            sendSubscriptions("subscribe", added);
    }

    bool OKXTickerStream::sendSubscriptions(const QString& operation, const Instruments& identifiers)
    {
        QJsonArray arguments;
        Instruments batch;
        qsizetype size = 0;
        for (const InstIdentifier& identifier : identifiers)
        {
            const QString instId = QString::fromStdString(identifier.id);
            if (instId.isEmpty() || instId.size() > 1000)
            {
                if (operation == "subscribe")
                    emit subscriptionFailed(identifier, "Invalid OKX instrument ID");
                continue;
            }

            const QJsonObject argument = TICKERS_CHANNEL.createArgument({{"instId", instId}});
            const qsizetype argumentSize = QJsonDocument(argument).toJson(QJsonDocument::Compact).size() + 1;
            if (size + argumentSize > 60000 && !arguments.isEmpty())
            {
                if (!sendBatch(operation, arguments, batch))
                    return false;
                arguments = {};
                batch.clear();
                size = 0;
            }
            arguments.append(argument);
            batch.insert(identifier);
            size += argumentSize;
        }
        return arguments.isEmpty() || sendBatch(operation, arguments, batch);
    }

    bool OKXTickerStream::sendBatch(const QString& operation, const QJsonArray& arguments, const Instruments& batch)
    {
        const QString id = QString::number(nextRequestId++);
        const QJsonDocument request(QJsonObject{{"id", id}, {"op", operation}, {"args", arguments}});
        if (!client.sendText(QString::fromUtf8(request.toJson(QJsonDocument::Compact))))
            return false;
        if (operation == "subscribe")
            pendingSubscriptions.emplace(id, batch);
        return true;
    }

    void OKXTickerStream::handleMessage(const QString& message)
    {
        try
        {
            const elementResult response = parseElement(message.toUtf8());
            std::string_view event;
            if (response["event"].get_string().get(event) == simdjson::SUCCESS)
            {
                if (event == "notice")
                {
                    client.reconnect(QString::fromUtf8(readString(response, "msg")));
                    return;
                }

                std::string_view id;
                if (response["id"].get_string().get(id) != simdjson::SUCCESS)
                {
                    if (event == "error")
                        emit connectionChanged(QString::fromUtf8(readString(response, "msg")));
                    return;
                }

                const std::map<QString, Instruments>::iterator request = pendingSubscriptions.find(QString::fromUtf8(id));
                if (request == pendingSubscriptions.end())
                    return;

                if (event == "error")
                {
                    const QString error = QString::fromUtf8(readString(response, "msg"));
                    for (const InstIdentifier& identifier : request->second)
                    {
                        if (instruments.contains(identifier))
                            emit subscriptionFailed(identifier, error);
                    }
                    pendingSubscriptions.erase(request);
                }
                else if (event == "subscribe")
                {
                    const std::string_view instId = readString(response["arg"], "instId");
                    std::erase_if(request->second, [instId](const InstIdentifier& identifier)
                    {
                        return identifier.id == instId;
                    });
                    if (request->second.empty())
                        pendingSubscriptions.erase(request);
                }
                return;
            }

            if (readString(response["arg"], "channel") != "tickers")
                return;
            for (const jsonElement row : OKXParser::getDataField(response))
            {
                const TickerData ticker = OKXParser::parseTickerFromElement(row);
                if (instruments.contains({Exchange::OKX, ticker.instType, ticker.instId}))
                    emit tickerReceived(ticker);
            }
        }
        catch (const simdjson::simdjson_error&)
        {
            // Ignore malformed messages and tickers without a complete quote.
        }
        catch (const std::runtime_error&)
        {
            // OKX can send empty quote fields before the first trade.
        }
    }
}
