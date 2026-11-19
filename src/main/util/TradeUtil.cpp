#include <cmath>
#include <QDateTime>
#include <QTimeZone>
#include <QStringList>
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
#include <QDateTime>
#include <QTimeZone>

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

namespace trttrade
{
    QString accountHistorySide(const PositionHistoryData& position)
    {
        const std::string& side = position.direction.empty() ? position.posSide : position.direction;
        if (side == "long") return "Long";
        if (side == "short") return "Short";
        return "—";
    }

    QString accountTime(const std::string& value)
    {
        bool valid = false;
        const qint64 timestamp = QString::fromStdString(value).toLongLong(&valid);
        return valid && timestamp > 0 ? QDateTime::fromMSecsSinceEpoch(timestamp, QTimeZone::UTC).toString("yyyy-MM-dd HH:mm:ss") : QString();
    }

    QString accountNumber(const std::string& value, bool signedValue)
    {
        if (value.empty()) return {};
        bool valid = false;
        const double parsed = QString::fromStdString(value).toDouble(&valid);
        if (!valid || !std::isfinite(parsed)) return "—";
        const QString rounded = trtutils::formatDecimal(parsed, 8);
        const QString formatted = rounded == "0" && parsed != 0 ? trtutils::formatQuantity(parsed) : rounded;
        return signedValue && parsed > 0 && formatted != "0" ? "+" + formatted : formatted;
    }

    QString accountPercent(const std::string& value)
    {
        if (value.empty()) return {};
        bool valid = false;
        const double parsed = QString::fromStdString(value).toDouble(&valid);
        return valid && std::isfinite(parsed) ? trtutils::formatDecimal(parsed * 100, 2) + "%" : QString("—");
    }

    QString accountPositionSide(const PositionData& position)
    {
        if (position.posSide == "long") return "Long";
        if (position.posSide == "short") return "Short";
        if (position.posSide != "net") return QString::fromStdString(position.posSide);
        if (position.instType == "MARGIN")
        {
            const QStringList parts = QString::fromStdString(position.instId).split('-');
            if (parts.size() >= 2 && position.posCcy == parts[0].toStdString()) return "Long";
            if (parts.size() >= 2 && position.posCcy == parts[1].toStdString()) return "Short";
            return "Net";
        }
        bool valid = false;
        const double size = QString::fromStdString(position.pos).toDouble(&valid);
        if (!valid) return "Net";
        if (size > 0) return "Long";
        if (size < 0) return "Short";
        return "Flat";
    }
}
