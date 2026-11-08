//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_JSONUTIL_H
#define NTRT_JSONUTIL_H

#include <QByteArray>

#include "simdjson.h"

namespace trtdata
{
    template <typename Element>
    using jsonResult = simdjson::simdjson_result<Element>;
    using jsonElement = simdjson::dom::element;

    using elementResult = jsonResult<jsonElement>;
    using stringResult = jsonResult<std::string_view>;

    using jsonArray = simdjson::dom::array;
    using jsonBuilder = simdjson::builder::string_builder;

    inline simdjson::dom::parser parser{};

    inline jsonBuilder newBuilder()
    {
        return jsonBuilder{};
    }

    template<class Number>
    Number readNumber(const jsonElement& element, const char* key);

    std::string_view readString(const jsonElement& element, const char* key);
    std::string_view readString(const elementResult& element, const char* key);

    jsonArray readArray(const jsonElement& element, const char* key);
    jsonArray readArray(const elementResult& element, const char* key);

    elementResult parseElement(const QByteArray& json);

    template<class Element>
    jsonResult<Element> parseJson(const QByteArray& json);

    std::string prettyJson(std::string_view json);
} // trtutils

#endif //NTRT_JSONUTIL_H
