//
// Created by NHCM on 2026/11/7.
//

#include "OKXParser.h"
#include <simdjson.h>
#include <stdexcept>

#include "main/util/JsonUtil.h"

namespace trtdata::OKXParser
{
    std::string_view getInstType(InstType type)
    {
        switch (type)
        {
        case InstType::Spot:    return "SPOT";
        case InstType::Futures: return "FUTURES";
        case InstType::Option:  return "OPTION";
        case InstType::Events:  return "EVENTS";
        case InstType::Margin:  return "MARGIN";
        case InstType::Swap:    return "SWAP";
        }

        return "";
    }

    InstType readType(std::string_view type)
    {
        for (const auto candidate : {InstType::Spot, InstType::Margin, InstType::Swap,
                                     InstType::Futures, InstType::Option, InstType::Events})
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
            tickers->push_back(parseTickerFromElement(element));
        }
        return tickers;
    }
} // trtdata
