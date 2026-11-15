#include "TradeUtil.h"
#include "NumberUtil.h"
#include "main/trade/TradeDataDef.h"

namespace trtutils
{
    QString instrumentLabel(const trtdata::InstIdentifier& identifier)
    {
        return QString::fromStdString(identifier.id) + " · " + QString::fromUtf8(
                trtdata::getExchangeStr(identifier.exchange)) +
            " · " + QString::fromUtf8(trttrade::instDisplayStr(identifier.type));
    }
}

#include "main/api/AbstractApi.h"
#include <stdexcept>

namespace trtdata
{
    Exchange readExchange(std::string_view name)
    {
        for (Exchange exchange : trtcore::availableExchanges())
        {
            if (getExchangeStr(exchange) == name)
                return exchange;
        }
        throw std::invalid_argument("Unknown exchange: " + std::string(name));
    }
}

#include <algorithm>
#include <cmath>

namespace trtutils
{
    std::vector<trttrade::BookLevel> aggregateBook(const std::vector<trttrade::BookLevel>& levels, bool bids,
                                                int depth, double step, int rows)
    {
        std::vector<trttrade::BookLevel> result;
        if (depth <= 0 || rows <= 0 || !std::isfinite(step) || step < 0)
            return result;
        long double bucketSize = 0;
        const std::size_t count = std::min(levels.size(), static_cast<std::size_t>(depth));
        for (std::size_t index = 0; index < count; index++)
        {
            const trttrade::BookLevel& level = levels[index];
            const double price = step > 0 ? roundToStep(level.price, step, !bids) : level.price;
            if (!result.empty() && result.back().price == price)
            {
                bucketSize += level.size;
                result.back().size = static_cast<double>(bucketSize);
            }
            else
            {
                if (result.size() == static_cast<std::size_t>(rows))
                    break;
                bucketSize = level.size;
                result.push_back({price, level.size});
            }
        }
        return result;
    }
}

namespace trtutils
{
    std::optional<double> bookBuyShare(const std::vector<trttrade::BookLevel>& bids,
                                       const std::vector<trttrade::BookLevel>& asks, int depth)
    {
        if (depth <= 0)
            return std::nullopt;
        long double buy = 0;
        long double sell = 0;
        for (std::size_t index = 0; index < std::min(bids.size(), static_cast<std::size_t>(depth)); index++)
            buy += bids[index].size;
        for (std::size_t index = 0; index < std::min(asks.size(), static_cast<std::size_t>(depth)); index++)
            sell += asks[index].size;
        if (buy + sell <= 0)
            return std::nullopt;
        return static_cast<double>(buy / (buy + sell));
    }
}
