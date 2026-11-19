#include "main/util/CandleUtil.h"
#include <QDateTime>
#include <QTimeZone>
#include <iostream>
#include <limits>
#include <stdexcept>

static void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    try
    {
        trttrade::CandleData candle;
        candle.timestamp = 60000;
        candle.open = candle.high = candle.low = candle.close = 100;
        candle.volume = 12;
        candle.volumeCcy = 24;
        candle.volumeQuote = 1200;
        trtdata::TickerData ticker{};
        ticker.ts = 119999;
        ticker.last = 105;
        check(trtutils::applyLivePrice(candle, trttrade::CandleTimeframe::Minute1, ticker), "Current-bar preview rejected");
        check(candle.close == 105 && candle.high == 105 && candle.low == 100, "Price envelope incorrect");
        check(candle.volume == 12 && candle.volumeCcy == 24 && candle.volumeQuote == 1200 && candle.open == 100 && !candle.confirmed, "Canonical fields changed");
        ticker.last = 95;
        check(trtutils::applyLivePrice(candle, trttrade::CandleTimeframe::Minute1, ticker) && candle.low == 95, "Falling quote failed");
        ticker.ts = 120000;
        check(!trtutils::applyLivePrice(candle, trttrade::CandleTimeframe::Minute1, ticker) && candle.close == 95, "Quote crossed bar boundary");
        ticker.ts = 59999;
        check(!trtutils::applyLivePrice(candle, trttrade::CandleTimeframe::Minute1, ticker), "Old quote accepted");
        ticker.ts = 100000;
        ticker.last = std::numeric_limits<double>::quiet_NaN();
        check(!trtutils::applyLivePrice(candle, trttrade::CandleTimeframe::Minute1, ticker), "Invalid quote accepted");
        ticker.last = 105;
        candle.confirmed = true;
        check(!trtutils::applyLivePrice(candle, trttrade::CandleTimeframe::Minute1, ticker), "Confirmed bar changed");
        candle.confirmed = false;
        candle.timestamp = QDateTime(QDate(2024, 2, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();
        ticker.ts = QDateTime(QDate(2024, 3, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();
        check(!trtutils::applyLivePrice(candle, trttrade::CandleTimeframe::Month1, ticker), "Month boundary incorrect");
        ticker.ts--;
        check(trtutils::applyLivePrice(candle, trttrade::CandleTimeframe::Month1, ticker), "Leap-month quote rejected");
        std::cout << "Candle preview boundaries and canonical fields passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
