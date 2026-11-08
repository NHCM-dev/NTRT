//
// Created by NHCM on 2026/11/7.
//

#include "JsonUtil.h"
#include <cstdint>

namespace trtdata
{
    template<class Number>
    Number readNumber(const jsonElement& element, const char* key)
    {
        const auto value = element[key].get_string().value();
        Number number{};
        const auto end = value.data() + value.size();
        const auto [ptr, error] = std::from_chars(value.data(), end, number);
        if (error != std::errc{} || ptr != end || !std::isfinite(number))
            throw std::runtime_error(std::string("Invalid json number field: ") + key);
        return number;
    }

    template double readNumber<double>(const jsonElement&, const char*);
    template std::int64_t readNumber<std::int64_t>(const jsonElement&, const char*);

    std::string_view readString(const jsonElement& element, const char* key)
    {
        return element[key].get_string().value();
    }
    std::string_view readString(const elementResult& element, const char* key)
    {
        return element[key].get_string().value();
    }

    jsonArray readArray(const jsonElement& element, const char* key)
    {
        return element[key].get_array().value();
    }
    jsonArray readArray(const elementResult& element, const char* key)
    {
        return element[key].get_array().value();
    }

    elementResult parseElement(const QByteArray& json)
    {
        return parseJson<jsonElement>(json);
    }

    template<class Element>
    jsonResult<Element> parseJson(const QByteArray& json)
    {
        return parser.parse(json.constData(), static_cast<std::size_t>(json.size())).value();
    }

    std::string prettyJson(std::string_view json)
    {
        jsonElement element;

        simdjson::error_code error = parser.parse(simdjson::padded_string(json)).get(element);

        if (error)
        {
            throw simdjson::simdjson_error(error);
        }

        return simdjson::prettify(element);
    }
} // trtutils
