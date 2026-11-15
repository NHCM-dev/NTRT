#include "OKXCandleStream.h"
#include "main/trade/parser/OKXParser.h"
#include "OKXApi.h"
#include "main/network/WebSocketAPITemplate.h"
#include <QJsonDocument>
#include <QJsonArray>

namespace trtdata::okx
{
    static const QUrl CANDLE_ENDPOINT{"wss://ws.okx.com/ws/v5/business"};

    OKXCandleStream::OKXCandleStream() : client(CANDLE_ENDPOINT, "ping", "pong")
    {
        connect(&client, &trtnet::WebSocketClient::connected, this, [this]
        {
            client.sendText(QString::fromUtf8(
                QJsonDocument(QJsonObject{{"op", "subscribe"}, {"args", QJsonArray{argument}}}).toJson(
                    QJsonDocument::Compact)));
        });
        connect(&client, &trtnet::WebSocketClient::connectionFailed, this, &CandleStream::connectionChanged);
        connect(&client, &trtnet::WebSocketClient::messageReceived, this, &OKXCandleStream::handleMessage);
    }

    void OKXCandleStream::start(const InstIdentifier& instrument, trttrade::CandleTimeframe timeframe,
                                TradingDataTimeZone zone)
    {
        const trtnet::WebSocketAPITemplate channel{CANDLE_ENDPOINT, "candle" + OKXParser::candleBar(timeframe, zone), {"instId"}};
        argument = channel.createArgument({{"instId", QString::fromStdString(instrument.id)}});
        client.start();
    }

    void OKXCandleStream::stop()
    {
        client.stop();
    }

    void OKXCandleStream::handleMessage(const QString& message)
    {
        const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());
        const QJsonObject object = document.object();
        if (object.value("event").toString() == "notice")
        {
            client.reconnect(object.value("msg").toString());
            return;
        }
        if (object.value("event").toString() == "error")
        {
            client.reconnect(object.value("msg").toString());
            return;
        }
        if (object.value("event").toString() == "subscribe" && object.value("arg").toObject() == argument)
        {
            emit connectionChanged({});
            return;
        }
        if (object.value("arg").toObject() != argument || !object.contains("data"))
            return;
        try
        {
            const QByteArray body = QJsonDocument(QJsonObject{{"code", "0"}, {"data", object.value("data")}}).toJson(
                QJsonDocument::Compact);
            emit candlesReceived(OKXParser::parseCandles(body));
        } catch (const std::exception& error)
        {
            client.reconnect(QString::fromUtf8(error.what()));
        }
    }
}
