#ifndef NTRT_MARKETCONFIGDEF_H
#define NTRT_MARKETCONFIGDEF_H

#include "main/data/config/TradingConfig.h"
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
        TradingConfig tradingData;
    };
}

#endif //NTRT_MARKETCONFIGDEF_H
