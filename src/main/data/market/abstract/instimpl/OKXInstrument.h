//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_OKXINSTRUMENT_H
#define NTRT_OKXINSTRUMENT_H
#include "main/data/market/abstract/Instrument.h"

namespace trtdata::okx
{
    class OKXInstrument final : public Instrument
    {
    public:
        OKXInstrument(const std::string& instId, InstType type)
            : Instrument(instId, type, Exchange::OKX)
        {}

        QFuture<TickerData> requestTicker() override;

        std::string getInstTypeStr() const override;
    };
} // trtdata

#endif //NTRT_OKXINSTRUMENT_H
