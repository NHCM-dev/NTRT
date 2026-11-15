#include "main/api/AbstractApi.h"
#include "main/monitor/CandlesManager.h"
#include "main/util/NumberUtil.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPromise>
#include <QThread>
#include <functional>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void waitFor(const std::function<bool()>& condition, const char* message, int timeout = 2500)
{
    QElapsedTimer timer;
    timer.start();
    while (!condition() && timer.elapsed() < timeout)
    {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    require(condition(), message);
}

trttrade::CandleData candle(qint64 minute, double price = 10, bool confirmed = false)
{
    return {minute * 60000, price, price, price, price, price, price, price, confirmed};
}

std::vector<trttrade::CandleData> candles(qint64 first, qint64 last)
{
    std::vector<trttrade::CandleData> result;
    for (qint64 minute = first; minute <= last; minute++)
        result.push_back(candle(minute));
    return result;
}

class FakeCandleStream final : public trtcore::CandleStream
{
public:
    void start(const trtdata::InstIdentifier&, trttrade::CandleTimeframe, trtdata::TradingDataTimeZone) override {}
    void stop() override {}
    void acknowledge() { emit connectionChanged({}); }
    void fail() { emit connectionChanged("Subscription rejected"); }
    void publish(const std::vector<trttrade::CandleData>& data) { emit candlesReceived(data); }
};

class FakeApi final : public trtcore::AbstractApi
{
public:
    struct Request
    {
        int limit;
        std::optional<qint64> after;
        std::shared_ptr<QPromise<std::vector<trttrade::CandleData>>> promise;
    };

private:
    mutable std::vector<Request> requests;
    mutable FakeCandleStream* stream = nullptr;

public:
    std::size_t getRequestCount() const { return requests.size(); }
    const Request& getRequest(std::size_t index) const { return requests.at(index); }
    FakeCandleStream* getStream() const { return stream; }

    int getRequestInterval() const override { return 100; }
    std::vector<trttrade::InstType> instrumentTypes() const override { return {}; }
    std::unique_ptr<trtcore::TickerStream> createTickerStream() const override { return {}; }
    std::unique_ptr<trtcore::CandleStream> createCandleStream() const override
    {
        std::unique_ptr<FakeCandleStream> result = std::make_unique<FakeCandleStream>();
        stream = result.get();
        return result;
    }

    QFuture<std::vector<trttrade::CandleData>> request(int limit, std::optional<qint64> after) const
    {
        std::shared_ptr<QPromise<std::vector<trttrade::CandleData>>> promise = std::make_shared<QPromise<std::vector<trttrade::CandleData>>>();
        promise->start();
        requests.push_back({limit, after, promise});
        return promise->future();
    }

    QFuture<std::vector<trttrade::CandleData>> fetchCandles(const trtdata::InstIdentifier&, trttrade::CandleTimeframe,
        trtdata::TradingDataTimeZone, int limit) const override { return request(limit, std::nullopt); }
    QFuture<std::vector<trttrade::CandleData>> fetchCandleHistory(const trtdata::InstIdentifier&, trttrade::CandleTimeframe,
        trtdata::TradingDataTimeZone, qint64 after) const override { return request(300, after); }
    QFuture<std::shared_ptr<std::vector<trtdata::TickerData>>> fetchTickers(trttrade::InstType) const override { return {}; }
    QFuture<std::vector<std::shared_ptr<trtdata::Instrument>>> fetchInstruments(trttrade::InstType) const override { return {}; }
    std::shared_ptr<trtdata::Instrument> createInstrument(const std::string&, trttrade::InstType) const override { return {}; }
    trttrade::InstType parseInstrumentType(std::string_view) const override { return trttrade::InstType::Spot; }

    void finish(std::size_t index, const std::vector<trttrade::CandleData>& data)
    {
        requests.at(index).promise->addResult(data);
        requests.at(index).promise->finish();
    }
};

FakeApi fakeApi;

namespace trtcore
{
    const AbstractApi& exchangeApi(trtdata::Exchange) { return fakeApi; }
    std::vector<trtdata::Exchange> availableExchanges() { return {trtdata::Exchange::OKX}; }
}

void testRecovery()
{
    trtcore::CandlesManager manager;
    const trtdata::InstIdentifier instrument{trtdata::Exchange::OKX, trttrade::InstType::Spot, "BTC-USDT"};
    const trttrade::CandleTimeframe timeframe = trttrade::CandleTimeframe::Minute1;
    manager.subscribe(instrument, timeframe);
    waitFor([] { return fakeApi.getRequestCount() == 1; }, "initial request");
    require(fakeApi.getRequest(0).limit == 300, "initial request loads full window");
    fakeApi.getStream()->acknowledge();
    fakeApi.finish(0, candles(1, 100));
    waitFor([] { return fakeApi.getRequestCount() == 2; }, "acknowledgement during request schedules fresh recovery");
    fakeApi.finish(1, candles(1, 100));
    waitFor([&manager, &instrument, timeframe] { return manager.getCandles(instrument, timeframe).size() == 100; }, "initial candles");
    QCoreApplication::processEvents();

    fakeApi.getStream()->fail();
    fakeApi.getStream()->acknowledge();
    // A live push arrives before REST scheduling and jumps over more than one full REST page.
    fakeApi.getStream()->publish({candle(800, 20)});
    waitFor([] { return fakeApi.getRequestCount() == 3; }, "reconnect request");
    require(fakeApi.getRequest(2).limit == 300 && !fakeApi.getRequest(2).after, "live update cannot shrink recovery window");
    fakeApi.getStream()->publish({candle(800, 30), candle(801, 40)});
    fakeApi.finish(2, candles(501, 800));
    waitFor([] { return fakeApi.getRequestCount() == 4; }, "first recovery history page");
    require(fakeApi.getRequest(3).after == 501 * 60000, "recovery cursor uses oldest fetched bar");
    require(manager.getCandle(instrument, timeframe, 800 * 60000)->close == 30, "delayed REST cannot overwrite live close");
    require(manager.getCandle(instrument, timeframe, 800 * 60000)->volume == 30, "delayed REST cannot overwrite live volume");
    require(manager.getCandle(instrument, timeframe, 801 * 60000)->close == 40, "bar rollover retained");
    fakeApi.finish(3, candles(201, 500));
    waitFor([] { return fakeApi.getRequestCount() == 5; }, "second recovery history page");
    require(fakeApi.getRequest(4).after == 201 * 60000, "recovery continues toward disconnect anchor");
    fakeApi.finish(4, candles(1, 200));
    waitFor([&manager, &instrument, timeframe] { return manager.getCandles(instrument, timeframe).size() == 801; }, "all missing bars recovered");

    manager.requestHistory(instrument, timeframe);
    waitFor([] { return fakeApi.getRequestCount() == 6; }, "manual history request");
    fakeApi.getStream()->publish({candle(801, 50)});
    std::vector<trttrade::CandleData> history{candle(0), candle(801, 15)};
    fakeApi.finish(5, history);
    waitFor([&manager, &instrument, timeframe] { return !manager.getSnapshot(instrument, timeframe)->historyLoading; }, "manual history completes");
    require(manager.getCandle(instrument, timeframe, 801 * 60000)->close == 50, "history response cannot overwrite newer live bar");

    fakeApi.getStream()->fail();
    waitFor([] { return fakeApi.getRequestCount() == 7; }, "failed subscription enables REST fallback");
    fakeApi.finish(6, {candle(801, 60)});
    waitFor([&manager, &instrument, timeframe] { return manager.getCandle(instrument, timeframe, 801 * 60000)->close == 60; }, "fallback data applied");
    require(!manager.getSnapshot(instrument, timeframe)->error.isEmpty(), "REST success preserves subscription error");
    waitFor([] { return fakeApi.getRequestCount() == 8; }, "REST fallback continues without subscription acknowledgement", 12000);
    require(fakeApi.getRequest(7).limit == 3, "steady fallback is incremental");

    // Changing zones cancels old requests. Their completion must not restore old data.
    manager.setTimeZone(trtdata::TradingDataTimeZone::UTC8);
    fakeApi.finish(7, {candle(900)});
    waitFor([] { return fakeApi.getRequestCount() == 9; }, "time zone reset requests new data");
    require(manager.getCandles(instrument, timeframe).empty(), "cancelled response cannot cross time zones");
    fakeApi.finish(8, {candle(1000)});
    waitFor([&manager, &instrument, timeframe] { return manager.getCandles(instrument, timeframe).size() == 1; }, "new zone data applied");
    manager.unsubscribe(instrument, timeframe);
    require(!manager.getSnapshot(instrument, timeframe), "released chart removes its series");
    manager.stop();
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try
    {
        testRecovery();
        require(trtutils::formatPrice(1.2345) == "1.2345", "retain fractional prices above one");
        require(trtutils::formatPrice(0.00123456) == "0.00123456", "retain small-price precision");
        std::cout << "candle recovery checks passed\n";
        return 0;
    } catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
