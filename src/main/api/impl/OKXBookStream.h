#ifndef NTRT_OKXBOOKSTREAM_H
#define NTRT_OKXBOOKSTREAM_H

#include "main/api/BookStream.h"
#include "main/network/WebSocketClient.h"
#include <map>

namespace trtdata::okx
{
    class OKXBookStream final : public trtcore::BookStream
    {
        trtnet::WebSocketClient client;
        Instruments instruments;
        std::map<InstIdentifier, trttrade::BookSnapshot> books;

        void sendSubscription(const QString& operation, const InstIdentifier& instrument);
        void handleMessage(const QString& message);
        void invalidate(const QString& reason);

    public:
        OKXBookStream();
        void setInstruments(const Instruments& desired) override;
        void stop() override;
    };
}
#endif
