#include "main/util/exchange/OKXBookUtil.h"
#include "main/util/NumberUtil.h"
#include <algorithm>
#include <map>

namespace trtdata::okx
{
    std::vector<trttrade::BookLevel> mergeBookSide(const std::vector<trttrade::BookLevel>& previous,
                                                const jsonArray& changes, bool bids)
    {
        std::map<double, double> levels;
        for (const trttrade::BookLevel& level : previous)
            levels.emplace(level.price, level.size);
        for (const jsonElement entry : changes)
        {
            const jsonArray fields = entry.get_array().value();
            const double price = trtutils::parseNumber<double>(fields.at(0).get_string().value(), "Invalid book price");
            const double size = trtutils::parseNumber<double>(fields.at(1).get_string().value(), "Invalid book size");
            if (price <= 0 || size < 0)
                throw std::runtime_error("Invalid book level");
            if (size == 0)
                levels.erase(price);
            else
                levels.insert_or_assign(price, size);
        }
        std::vector<trttrade::BookLevel> result;
        for (const std::pair<const double, double>& level : levels)
            result.push_back({level.first, level.second});
        if (bids)
            std::reverse(result.begin(), result.end());
        if (result.size() > 400)
            result.resize(400);
        return result;
    }

    bool applyBookMessage(const jsonElement& row, bool snapshot, trttrade::BookSnapshot& book)
    {
        const qint64 sequence = row["seqId"].get_int64().value();
        const qint64 previous = row["prevSeqId"].get_int64().value();
        if (sequence < 0 || (snapshot && previous != -1))
            return false;
        if (!snapshot && (book.sequence < 0 || previous != book.sequence))
            return false;
        trttrade::BookSnapshot next;
        next.bids = mergeBookSide(snapshot ? std::vector<trttrade::BookLevel>{} : book.bids, readArray(row, "bids"), true);
        next.asks = mergeBookSide(snapshot ? std::vector<trttrade::BookLevel>{} : book.asks, readArray(row, "asks"), false);
        if (!next.bids.empty() && !next.asks.empty() && next.bids.front().price >= next.asks.front().price)
            return false;
        next.sequence = sequence;
        next.updatedAt = trtutils::parseNumber<qint64>(readString(row, "ts"), "Invalid book timestamp");
        book = std::move(next);
        return true;
    }
}
