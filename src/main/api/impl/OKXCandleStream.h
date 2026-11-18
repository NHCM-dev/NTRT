#ifndef NTRT_OKXCANDLESTREAM_H
#define NTRT_OKXCANDLESTREAM_H
#include "main/api/CandleStream.h"
#include "main/network/WebSocketClient.h"
#include <QJsonObject>

namespace trtdata::okx
{
    class OKXCandleStream final : public trtcore::CandleStream
    {
        trtnet::WebSocketClient client;
        QJsonObject argument;

        void handleMessage(const QString& message);

    public:
        OKXCandleStream();

        void start(const InstIdentifier& instrument, trttrade::CandleTimeframe timeframe,
                   TradingTimeZone zone) override;

        void stop() override;
    };
}
#endif
