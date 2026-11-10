#ifndef NTRT_OKXTICKERSTREAM_H
#define NTRT_OKXTICKERSTREAM_H

#include "TickerStream.h"
#include "main/network/WebSocketClient.h"
#include <QJsonArray>
#include <map>

namespace trtdata::okx
{
    class OKXTickerStream final : public trtcore::TickerStream
    {
        trtnet::WebSocketClient client;
        Instruments instruments;
        std::map<QString, Instruments> pendingSubscriptions;
        quint64 nextRequestId = 0;

        void initClient();
        void syncSubscriptions(const Instruments& previous);
        bool sendSubscriptions(const QString& operation, const Instruments& arguments);
        bool sendBatch(const QString& operation, const QJsonArray& arguments, const Instruments& batch);
        void handleMessage(const QString& message);

    public:
        OKXTickerStream();
        void setInstruments(const Instruments& instruments) override;
        void stop() override;
    };
}

#endif //NTRT_OKXTICKERSTREAM_H
