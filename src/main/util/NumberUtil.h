#ifndef NTRT_NUMBERUTIL_H
#define NTRT_NUMBERUTIL_H

#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>

namespace trtutils
{
    template<class Number>
    Number parseNumber(std::string_view value, std::string_view errorMessage)
    {
        if (value.empty())
            throw std::runtime_error(std::string(errorMessage));
        Number number{};
        const char* end = value.data() + value.size();
        const std::from_chars_result parsed = std::from_chars(value.data(), end, number);
        if (parsed.ec != std::errc{} || parsed.ptr != end || !std::isfinite(number))
            throw std::runtime_error(std::string(errorMessage));
        return number;
    }
}

#endif //NTRT_NUMBERUTIL_H
