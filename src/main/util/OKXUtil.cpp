#include "OKXUtil.h"

namespace trtdata::okx
{
    QString candleBar(trttrade::CandleTimeframe timeframe, TradingDataTimeZone timeZone)
    {
        QString bar;
        bool calendar = false;
        switch (timeframe)
        {
            case trttrade::CandleTimeframe::Minute1:
                bar = "1m";
                break;
            case trttrade::CandleTimeframe::Minute3:
                bar = "3m";
                break;
            case trttrade::CandleTimeframe::Minute5:
                bar = "5m";
                break;
            case trttrade::CandleTimeframe::Minute15:
                bar = "15m";
                break;
            case trttrade::CandleTimeframe::Minute30:
                bar = "30m";
                break;
            case trttrade::CandleTimeframe::Hour1:
                bar = "1H";
                break;
            case trttrade::CandleTimeframe::Hour2:
                bar = "2H";
                break;
            case trttrade::CandleTimeframe::Hour4:
                bar = "4H";
                break;
            case trttrade::CandleTimeframe::Day1:
                bar = "1D";
                calendar = true;
                break;
            case trttrade::CandleTimeframe::Week1:
                bar = "1W";
                calendar = true;
                break;
            case trttrade::CandleTimeframe::Month1:
                bar = "1M";
                calendar = true;
                break;
        }
        if (calendar)
        {
            switch (timeZone)
            {
                case TradingDataTimeZone::UTC:
                    bar += "utc";
                    break;
                case TradingDataTimeZone::UTC8:
                    break;
            }
        }
        return bar;
    }
}
