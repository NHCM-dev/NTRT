#include "main/api/AbstractApi.h"
#include "main/trade/parser/OKXParser.h"
#include "main/data/AppearanceManager.h"
#include "main/data/ConfigManager.h"
#include "main/util/JsonUtil.h"
#include <QFile>
#include "main/monitor/CandlesManager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QPromise>
#include <QTemporaryDir>
#include <QTimer>
#include <functional>
#include <iostream>
#include <stdexcept>

class CandleTests final : public trtcore::AbstractApi
{
    mutable std::vector<std::shared_ptr<QPromise<std::vector<trttrade::CandleData>>>> requests;
    mutable std::vector<int> limits;
    mutable std::vector<trttrade::CandleTimeframe> timeframes;
    mutable std::vector<trtdata::TradingDataTimeZone> timeZones;

    static void require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    static void waitFor(const std::function<bool()>& ready)
    {
        QElapsedTimer elapsed;
        elapsed.start();
        while (!ready() && elapsed.elapsed() < 2000)
        {
            QEventLoop loop;
            QTimer::singleShot(10, &loop, &QEventLoop::quit);
            loop.exec();
        }
        require(ready(), "Timed out waiting for candle update");
    }

    static void testParser()
    {
        const QByteArray valid = R"({"code":"0","data":[["86400000","100","120","90","110","2","3","4","0"],["0","90","100","80","100","5","6","7","1"]]})";
        const std::vector<trttrade::CandleData> candles = trtdata::OKXParser::parseCandles(valid);
        require(candles.size() == 2 && candles.front().timestamp == 0 && candles.back().timestamp == 86400000, "Candle order is incorrect");
        require(candles.front().confirmed && !candles.back().confirmed, "Candle confirmation is incorrect");
        require(candles.back().volume == 2 && candles.back().volumeCcy == 3 && candles.back().volumeQuote == 4, "Candle volume units were mixed");
        for (const QByteArray& invalid : {
                 QByteArray(R"({"code":"50011","msg":"Rate limit","data":[]})"),
                 QByteArray(R"({"code":"0","data":[["0","100"]]})"),
                 QByteArray(R"({"code":"0","data":[["0","nan","120","90","110","2","3","4","0"]]})"),
                 QByteArray(R"({"code":"0","data":[["0","100x","120","90","110","2","3","4","0"]]})"),
                 QByteArray(R"({"code":"0","data":[["0","100","90","120","110","2","3","4","0"]]})"),
                 QByteArray(R"({"code":"0","data":[["0","100","120","90","110","-2","3","4","0"]]})")})
        {
            bool rejected = false;
            try { trtdata::OKXParser::parseCandles(invalid); }
            catch (const std::exception&) { rejected = true; }
            require(rejected, "Invalid candle response was accepted");
        }
    }

    static void testDefaults()
    {
        QTemporaryDir directory;
        require(directory.isValid(), "Could not create settings directory");
        trtdata::AppearanceManager settings(directory.path());
        require(settings.tryInitOnce() && settings.loadConfig(), "Could not initialize default settings");
        require(settings.getCurrentTheme() == trtdata::EnumTheme::Dark, "Default theme is not dark");
        require(!settings.getMarketColumns().type && !settings.getMarketColumns().volume24h, "Default columns are incorrect");
        trtdata::MarketColumnVisibility columns{true, true, true};
        require(settings.setMarketColumns(columns), "Could not save existing column choices");
        settings.setTheme(trtdata::EnumTheme::Light);
        require(settings.saveConfig(), "Could not save existing theme");
        trtdata::AppearanceManager restored(directory.path());
        require(restored.loadConfig() && restored.getCurrentTheme() == trtdata::EnumTheme::Light && restored.getMarketColumns().type,
                "Existing preferences were overwritten by defaults");

        trtdata::ConfigManager config(directory.path());
        require(config.getTradingDataTimeZone() == trtdata::TradingDataTimeZone::UTC, "Default trading time zone is not UTC");
        const QString marketPath = directory.path() + "/market.json";
        require(trtdata::writeJsonFile(marketPath, R"({"instruments":[]})") && config.loadMarketConfig(), "Legacy market configuration did not load");
        require(config.getTradingDataTimeZone() == trtdata::TradingDataTimeZone::UTC, "Legacy configuration did not default to UTC");
        require(config.setTradingDataTimeZone(trtdata::TradingDataTimeZone::UTC8), "Could not save trading data time zone");
        trtdata::ConfigManager loaded(directory.path());
        require(loaded.loadMarketConfig() && loaded.getTradingDataTimeZone() == trtdata::TradingDataTimeZone::UTC8, "Trading time zone did not persist");
        require(trtdata::writeJsonFile(marketPath, R"({"instruments":[],"tradingData":{"timeZone":"invalid"}})"), "Could not write invalid settings fixture");
        require(!loaded.loadMarketConfig() && loaded.getTradingDataTimeZone() == trtdata::TradingDataTimeZone::UTC8, "Invalid configuration replaced valid settings");
        QFile obstruction(directory.path() + "/blocked");
        require(obstruction.open(QIODevice::WriteOnly), "Could not create failed-save fixture");
        obstruction.close();
        trtdata::ConfigManager blocked(obstruction.fileName());
        require(!blocked.setTradingDataTimeZone(trtdata::TradingDataTimeZone::UTC8) && blocked.getTradingDataTimeZone() == trtdata::TradingDataTimeZone::UTC,
                "Failed save changed the active time zone");
    }

    void complete(std::size_t index, const std::vector<trttrade::CandleData>& candles)
    {
        requests[index]->addResult(candles);
        requests[index]->finish();
    }

public:
    int getRequestInterval() const override { return 100; }
    std::vector<trttrade::InstType> instrumentTypes() const override { return {}; }
    std::unique_ptr<trtcore::TickerStream> createTickerStream() const override { return {}; }
    QFuture<std::shared_ptr<std::vector<trtdata::TickerData>>> fetchTickers(trttrade::InstType) const override { return {}; }
    QFuture<std::vector<std::shared_ptr<trtdata::Instrument>>> fetchInstruments(trttrade::InstType) const override { return {}; }
    std::shared_ptr<trtdata::Instrument> createInstrument(const std::string&, trttrade::InstType) const override { return {}; }
    trttrade::InstType parseInstrumentType(std::string_view) const override { return trttrade::InstType::Spot; }
    QFuture<std::vector<trttrade::CandleData>> fetchCandles(const trtdata::InstIdentifier&, trttrade::CandleTimeframe timeframe, trtdata::TradingDataTimeZone timeZone, int limit) const override
    {
        timeZones.push_back(timeZone);
        timeframes.push_back(timeframe);
        std::shared_ptr<QPromise<std::vector<trttrade::CandleData>>> promise = std::make_shared<QPromise<std::vector<trttrade::CandleData>>>();
        promise->start();
        requests.push_back(promise);
        limits.push_back(limit);
        return promise->future();
    }

    QFuture<std::vector<trttrade::CandleData>> fetchCandleHistory(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe, trtdata::TradingDataTimeZone zone, qint64) const override
    {
        return fetchCandles(instrument, timeframe, zone, 300);
    }
    void run();
};

class TestInstrument final : public trtdata::Instrument
{
public:
    TestInstrument() : Instrument("BTC-USDT-SWAP", trttrade::InstType::Swap, trtdata::Exchange::OKX) {}
    QFuture<trtdata::TickerData> requestTicker() override { return {}; }
    std::string getInstTypeStr() const override { return "SWAP"; }
};

static CandleTests api;

namespace trtcore
{
    const AbstractApi& exchangeApi(trtdata::Exchange) { return api; }
    std::vector<trtdata::Exchange> availableExchanges() { return {trtdata::Exchange::OKX}; }
}

void CandleTests::run()
{
    testParser();
    testDefaults();
    trtcore::CandlesManager manager;
    const std::vector<std::shared_ptr<trtdata::Instrument>> instruments{std::make_shared<TestInstrument>()};
    const trtdata::InstIdentifier identifier = instruments.front()->getUniqueIdentifier();
    const qint64 dayMs = 86400000;
    const qint64 today = QDateTime::currentMSecsSinceEpoch() / dayMs * dayMs;
    std::vector<trttrade::CandleData> history;
    for (int index = 0; index < 300; index++)
        history.push_back({today - (300 - index) * dayMs, 100, 120, 90, 110, 2, 3, 4, true});

    manager.setInstruments(instruments);
    waitFor([this] { return requests.size() == 1; });
    require(limits.back() == 300, "Initial history request is incomplete");
    require(timeZones.back() == trtdata::TradingDataTimeZone::UTC, "Default candle time zone is not UTC");
    complete(0, history);
    waitFor([&manager, &identifier] { return manager.getSnapshot(identifier)->updatedAt != 0; });
    require(!manager.getDailyCandle(identifier), "Yesterday's candle was shown as today's");

    manager.stop();
    manager.setInstruments(instruments);
    waitFor([this] { return requests.size() == 2; });
    require(limits.back() == 3, "Incremental request did not reuse cached history");
    complete(1, {{today - dayMs, 100, 120, 90, 115, 5, 6, 7, true}, {today, 100, 120, 90, 110, 2, 3, 4, false}});
    waitFor([&manager, &identifier] { return manager.getDailyCandle(identifier) != nullptr; });
    const trtcore::CandleSnapshot* snapshot = manager.getSnapshot(identifier);
    require(snapshot->candles.size() == 300 && snapshot->candles.front().timestamp == today - 299 * dayMs, "History was not bounded after rollover");
    require(snapshot->candles[298].close == 115 && snapshot->candles.back().close == 110, "Incremental candle merge failed");

    manager.stop();
    manager.setInstruments(instruments);
    waitFor([this] { return requests.size() == 3; });
    requests[2]->setException(std::make_exception_ptr(std::runtime_error("offline")));
    requests[2]->finish();
    waitFor([&manager, &identifier] { return manager.getSnapshot(identifier)->error == "offline"; });
    require(manager.getDailyCandle(identifier)->close == 110, "Network failure discarded cached candles");

    manager.stop();
    manager.setInstruments(instruments);
    waitFor([this] { return requests.size() == 4; });
    manager.setInstruments({});
    require(!manager.getSnapshot(identifier), "Removed instrument retained its subscription");
    manager.setInstruments(instruments);
    waitFor([this] { return requests.size() == 5; });
    complete(3, {{today, 100, 120, 90, 119, 2, 3, 4, false}});
    require(manager.getSnapshot(identifier)->candles.empty(), "Removed request populated a new subscription");
    complete(4, {{today, 100, 120, 90, 112, 2, 3, 4, false}});
    waitFor([&manager, &identifier] { return manager.getDailyCandle(identifier) != nullptr; });
    require(manager.getDailyCandle(identifier)->close == 112, "Re-added instrument did not receive fresh data");
    const std::optional<double> increase = manager.getDailyChangePercent(identifier, 125);
    const std::optional<double> decrease = manager.getDailyChangePercent(identifier, 95);
    require(increase && *increase == 25 && decrease && *decrease == -5, "Daily change did not use the latest price");
    require(!manager.getDailyChangePercent(identifier, 0), "Missing latest price produced a daily change");

    manager.stop();
    manager.setInstruments(instruments);
    waitFor([this] { return requests.size() == 6; });
    manager.setTimeZone(trtdata::TradingDataTimeZone::UTC8);
    require(!manager.getDailyCandle(identifier) && manager.getSnapshot(identifier)->candles.empty(), "Time zone change retained old candles");
    waitFor([this] { return requests.size() == 7; });
    require(timeZones.back() == trtdata::TradingDataTimeZone::UTC8 && limits.back() == 300, "Time zone change did not reload history");
    complete(5, {{today, 100, 120, 90, 119, 2, 3, 4, false}});
    const qint64 utc8Today = (QDateTime::currentMSecsSinceEpoch() + 8 * 3600000) / dayMs * dayMs - 8 * 3600000;
    complete(6, {{utc8Today, 200, 240, 180, 210, 2, 3, 4, false}});
    waitFor([&manager, &identifier] { return manager.getDailyCandle(identifier) != nullptr; });
    require(manager.getDailyCandle(identifier)->open == 200, "Old time zone request overwrote new daily open");
    const std::optional<double> utc8Change = manager.getDailyChangePercent(identifier, 220);
    require(utc8Change && *utc8Change == 10, "New time zone daily change is incorrect");
    const trttrade::CandleTimeframe day = trttrade::CandleTimeframe::Day1;
    require(manager.getCandle(identifier, day, utc8Today)->open == 200, "Exact lookup failed");
    require(!manager.getCandle(identifier, day, utc8Today + 1), "Exact lookup returned a neighboring bar");
    require(manager.getCandles(identifier, day, utc8Today, utc8Today + 1).size() == 1, "Range lookup failed");
    require(manager.getCandles(identifier, day, utc8Today - 1, utc8Today).empty(), "Range end must be exclusive");
    require(manager.getLatestCandles(identifier, day, 0).empty(), "Zero count must return no bars");
    require(manager.getLatestCandles(identifier, day, 1000).size() == 1, "Latest count was not clamped");
    require(manager.getCandleMap(identifier, day).at(utc8Today).open == 200, "Map export failed");
    manager.subscribe(identifier, trttrade::CandleTimeframe::Minute1);
    waitFor([this] { return requests.size() == 8; });
    require(timeframes.back() == trttrade::CandleTimeframe::Minute1, "Timeframe was not passed to exchange API");
    complete(7, {{utc8Today, 10, 12, 9, 11, 2, 3, 4, false}});
    waitFor([&manager, &identifier] { return !manager.getCandles(identifier, trttrade::CandleTimeframe::Minute1).empty(); });
    require(manager.getCandle(identifier, day, utc8Today)->open == 200, "Timeframes contaminated each other");
    manager.unsubscribe(identifier, trttrade::CandleTimeframe::Minute1);
    require(manager.getCandles(identifier, trttrade::CandleTimeframe::Minute1).empty(), "Unsubscribe retained series");
    manager.subscribe(identifier, day);
    const std::size_t historyRequest = requests.size();
    manager.requestHistory(identifier, day);
    require(manager.getSnapshot(identifier, day)->historyLoading, "History loading state was not published");
    waitFor([this, historyRequest] { return requests.size() > historyRequest; });
    complete(historyRequest, {{utc8Today - dayMs, 100, 120, 90, 110, 2, 3, 4, true}});
    waitFor([&manager, &identifier, day] { return manager.getCandles(identifier, day).size() == 2; });
    require(manager.getCandles(identifier, day).front().timestamp == utc8Today - dayMs, "History page was not prepended");
    require(!manager.getSnapshot(identifier, day)->historyLoading, "Completed history request remained loading");
    manager.unsubscribe(identifier, day);
    manager.subscribe(identifier, trttrade::CandleTimeframe::Minute5);
    manager.subscribe(identifier, trttrade::CandleTimeframe::Minute5);
    manager.unsubscribe(identifier, trttrade::CandleTimeframe::Minute5);
    require(manager.getSnapshot(identifier, trttrade::CandleTimeframe::Minute5) != nullptr, "Closing one chart removed another chart's series");
    manager.setInstruments({});
    require(manager.getSnapshot(identifier, trttrade::CandleTimeframe::Minute5) != nullptr, "List removal discarded a chart subscription");
    manager.unsubscribe(identifier, trttrade::CandleTimeframe::Minute5);
    require(!manager.getSnapshot(identifier, trttrade::CandleTimeframe::Minute5), "Last reader retained an untracked series");
    manager.setInstruments(instruments);
    manager.subscribe(identifier, day);
    manager.unsubscribe(identifier, day);
    require(manager.getSnapshot(identifier) != nullptr, "Closing a daily chart removed the instrument list's daily series");
    manager.stop();
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try
    {
        api.run();
        std::cout << "Candle parser, daily rollover, cache merge, lifecycle, failure recovery and settings tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
