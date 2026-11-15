#ifndef NTRT_CONFIGDEF_H
#define NTRT_CONFIGDEF_H

#include "ExchangeDef.h"
#include "TradingDataDef.h"
#include <memory>
#include <string>
#include <vector>
#include "main/market/abstract/Instrument.h"

namespace trtdata
{
    struct MarketConfig
    {
        std::string fileName;
        std::vector<std::shared_ptr<Instrument>> instruments;
        TradingDataConfig tradingData;
    };

    struct ExchangeConfig
    {
        std::string fileName;
        Exchange current{};
    };
}

#endif //NTRT_CONFIGDEF_H
