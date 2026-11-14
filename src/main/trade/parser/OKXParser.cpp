//
// Created by NHCM on 2026/11/7.
//

#include "OKXParser.h"
#include <simdjson.h>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>

#include "main/util/JsonUtil.h"

namespace trtdata::OKXParser
{
    std::string_view getInstType(trttrade::InstType type)
    {
        switch (type)
        {
        case trttrade::InstType::Spot:    return "SPOT";
        case trttrade::InstType::Futures: return "FUTURES";
        case trttrade::InstType::Option:  return "OPTION";
        case trttrade::InstType::Events:  return "EVENTS";
        case trttrade::InstType::Margin:  return "MARGIN";
        case trttrade::InstType::Swap:    return "SWAP";
        }

        return "";
    }

    trttrade::InstType readType(std::string_view type)
    {
        for (const auto candidate : {trttrade::InstType::Spot, trttrade::InstType::Margin, trttrade::InstType::Swap,
                                     trttrade::InstType::Futures, trttrade::InstType::Option, trttrade::InstType::Events})
        {
            if (getInstType(candidate) == type)
                return candidate;
        }
        throw std::runtime_error("Unknown OKX instrument type: " + std::string(type));
    }

    void ensureSuccess(const elementResult& json)
    {
        const std::string_view code = readString(json, "code");
        if (code != "0")
        {
            std::string_view message;
            if (json["msg"].get_string().get(message) != simdjson::SUCCESS)
                message = {};
            throw std::runtime_error("OKX ticker error " + std::string(code) + ": " + std::string(message));
        }
    }

    jsonArray getDataField(const elementResult& json)
    {
        return readArray(json, "data");
    }

    const TickerData parseTickerFromElement(const jsonElement& json)
    {
        const std::string_view id = readString(json, "instId");
        if (id.empty())
            throw std::runtime_error("Empty OKX ticker instrument ID");

        return {
            .instType = readType(readString(json, "instType")),
            .instId = std::string(id),
            .last = readNumber<double>(json, "last"),
            .open24h = readNumber<double>(json, "open24h"),
            .high24h = readNumber<double>(json, "high24h"),
            .low24h = readNumber<double>(json, "low24h"),
            .volCcy24h = readNumber<double>(json, "volCcy24h"),
            .vol24h = readNumber<double>(json, "vol24h"),
            .ts = readNumber<std::int64_t>(json, "ts"),
        };
    }

    std::vector<trttrade::CandleData> parseCandles(const QByteArray& json)
    {
        const elementResult response = parseElement(json);
        ensureSuccess(response);
        const jsonArray rows = getDataField(response);
        std::vector<trttrade::CandleData> candles;
        candles.reserve(rows.size());
        for (const jsonElement row : rows)
        {
            const jsonArray fields = row.get_array().value();
            if (fields.size() != 9)
                throw std::runtime_error("Invalid OKX candle field count");

            std::array<std::string_view, 9> values;
            std::size_t index = 0;
            for (const jsonElement field : fields)
                values[index++] = field.get_string().value();

            trttrade::CandleData candle;
            const std::from_chars_result timestamp = std::from_chars(values[0].data(), values[0].data() + values[0].size(), candle.timestamp);
            if (timestamp.ec != std::errc{} || timestamp.ptr != values[0].data() + values[0].size() || candle.timestamp < 0)
                throw std::runtime_error("Invalid OKX candle timestamp");

            const std::array<double*, 7> numbers{&candle.open, &candle.high, &candle.low, &candle.close,
                                                &candle.volume, &candle.volumeCcy, &candle.volumeQuote};
            for (std::size_t number = 0; number < numbers.size(); number++)
            {
                const std::string_view value = values[number + 1];
                const std::from_chars_result parsed = std::from_chars(value.data(), value.data() + value.size(), *numbers[number]);
                if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || !std::isfinite(*numbers[number]) || *numbers[number] < 0)
                    throw std::runtime_error("Invalid OKX candle number");
            }
            if (values[8] != "0" && values[8] != "1")
                throw std::runtime_error("Invalid OKX candle confirmation");
            if (candle.low > candle.high || candle.open < candle.low || candle.open > candle.high ||
                candle.close < candle.low || candle.close > candle.high)
                throw std::runtime_error("Invalid OKX candle price range");
            candle.confirmed = values[8] == "1";
            candles.push_back(candle);
        }
        std::sort(candles.begin(), candles.end(), [](const trttrade::CandleData& left, const trttrade::CandleData& right) { return left.timestamp < right.timestamp; });
        for (std::size_t index = 1; index < candles.size(); index++)
        {
            if (candles[index - 1].timestamp == candles[index].timestamp)
                throw std::runtime_error("Duplicate OKX candle timestamp");
        }
        return candles;
    }

    const TickerData parseTicker(const QByteArray& json)
    {
        const elementResult response = parseElement(json);
        ensureSuccess(response);

        const jsonArray data = getDataField(response);
        if (data.size() != 1)
            throw std::runtime_error("Expected one OKX ticker in response data");
        const jsonElement ticker = *data.begin();
        return parseTickerFromElement(ticker);
    }

    const std::shared_ptr<std::vector<TickerData>> parseTickers(const QByteArray& json)
    {
        const elementResult response = parseElement(json);
        ensureSuccess(response);

        const jsonArray data = getDataField(response);
        std::shared_ptr<std::vector<TickerData>> tickers = std::make_shared<std::vector<TickerData>>();
        for (const jsonElement& element : data)
        {
            try
            {
                tickers->push_back(parseTickerFromElement(element));
            }
            catch (const simdjson::simdjson_error&)
            {
                // A ticker can omit quote fields before its first trade.
            }
            catch (const std::runtime_error&)
            {
                // Skip invalid quote fields without dropping the rest of the market.
            }
        }
        if (data.size() != 0 && tickers->empty())
            throw std::runtime_error("OKX returned no usable tickers");
        return tickers;
    }
} // trtdata
