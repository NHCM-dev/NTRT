#include "main/monitor/DataMonitor.h"
#include "main/data/ConfigManager.h"
#include "main/api/AbstractApi.h"
#include "main/network/RestAPITemplate.h"
#include "main/util/NetworkUtil.h"
#include "main/trade/parser/OKXParser.h"
#include <QTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <atomic>
#include <chrono>
#include <mutex>
#include <semaphore>

class MonitorTestApi;

class MonitorTestTicker final : public trtcore::TickerStream
{
public:
    void setInstruments(const Instruments&) override
    {
        QTimer::singleShot(0, this, [this]
        {
            emit tickerReceived({trttrade::InstType::Spot, "BTC-USDT", 100, 90, 120, 80, 10, 20, 1700000000000LL});
        });
    }
    void stop() override {}
};

class MonitorTestCandle final : public trtcore::CandleStream
{
    const MonitorTestApi& api;
    trtcore::CandlesManager::SeriesKey key{};
    bool active = false;

public:
    explicit MonitorTestCandle(const MonitorTestApi& api) : api(api) {}
    ~MonitorTestCandle() override;
    void start(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe, trtdata::TradingDataTimeZone) override;
    void stop() override;
    void send(double close)
    {
        emit candlesReceived({{1700000000000LL, 100, 140, 90, close, 2, 3, 4, false}});
    }
};

class MonitorTestApi final : public trtcore::AbstractApi
{
    mutable std::atomic<QThread*> requestThread{nullptr};
    mutable std::atomic<QThread*> parseThread{nullptr};
    mutable std::atomic<QThread*> tickerThread{nullptr};
    mutable std::atomic<const void*> parserAddress{nullptr};
    std::atomic<int> port{0};
    mutable std::mutex streamMutex;
    mutable std::map<trtcore::CandlesManager::SeriesKey, MonitorTestCandle*> streams;

public:
    void setPort(int value) { port.store(value); }
    QThread* getRequestThread() const { return requestThread.load(); }
    QThread* getParseThread() const { return parseThread.load(); }
    QThread* getTickerThread() const { return tickerThread.load(); }
    const void* getParserAddress() const { return parserAddress.load(); }
    void addStream(const trtcore::CandlesManager::SeriesKey& key, MonitorTestCandle* stream) const
    {
        const std::lock_guard<std::mutex> lock(streamMutex);
        streams[key] = stream;
    }
    void removeStream(const trtcore::CandlesManager::SeriesKey& key) const
    {
        const std::lock_guard<std::mutex> lock(streamMutex);
        streams.erase(key);
    }
    bool dispatch(const trtcore::CandlesManager::SeriesKey& key, const std::function<void(MonitorTestCandle*)>& action)
    {
        const std::lock_guard<std::mutex> lock(streamMutex);
        const std::map<trtcore::CandlesManager::SeriesKey, MonitorTestCandle*>::const_iterator entry = streams.find(key);
        if (entry == streams.end())
            return false;
        MonitorTestCandle* stream = entry->second;
        return QMetaObject::invokeMethod(stream, [stream, action] { action(stream); }, Qt::QueuedConnection);
    }
    bool hasStream(const trtcore::CandlesManager::SeriesKey& key)
    {
        const std::lock_guard<std::mutex> lock(streamMutex);
        return streams.contains(key);
    }
    int getRequestInterval() const override { return 100; }
    std::vector<trttrade::InstType> instrumentTypes() const override { return {}; }
    std::unique_ptr<trtcore::TickerStream> createTickerStream() const override
    {
        tickerThread.store(QThread::currentThread());
        return std::make_unique<MonitorTestTicker>();
    }
    std::unique_ptr<trtcore::CandleStream> createCandleStream() const override
    {
        return std::make_unique<MonitorTestCandle>(*this);
    }
    QFuture<std::shared_ptr<std::vector<trtdata::TickerData>>> fetchTickers(trttrade::InstType) const override { return {}; }
    QFuture<std::vector<std::shared_ptr<trtdata::Instrument>>> fetchInstruments(trttrade::InstType) const override { return {}; }
    std::shared_ptr<trtdata::Instrument> createInstrument(const std::string&, trttrade::InstType) const override { return {}; }
    trttrade::InstType parseInstrumentType(std::string_view) const override { return trttrade::InstType::Spot; }
    QFuture<std::vector<trttrade::CandleData>> fetchCandles(const trtdata::InstIdentifier&, trttrade::CandleTimeframe,
        trtdata::TradingDataTimeZone zone, int) const override
    {
        requestThread.store(QThread::currentThread());
        const trtnet::RestAPITemplate endpoint{trtnet::HTTPMethod::GET, QUrl("http://127.0.0.1:" + QString::number(port.load())), "/candles", {"zone"}};
        QUrlQuery query;
        query.addQueryItem("zone", QString::number(static_cast<int>(zone)));
        return trtnet::replyToFuture(endpoint.request(query), [this](const QByteArray& body)
        {
            parseThread.store(QThread::currentThread());
            parserAddress.store(&trtdata::parser);
            return trtdata::OKXParser::parseCandles(body);
        });
    }
};

static MonitorTestApi api;

MonitorTestCandle::~MonitorTestCandle() { stop(); }
void MonitorTestCandle::start(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe, trtdata::TradingDataTimeZone)
{
    key = {instrument, timeframe};
    active = true;
    api.addStream(key, this);
}
void MonitorTestCandle::stop()
{
    if (active)
        api.removeStream(key);
    active = false;
}

namespace trtcore
{
    const AbstractApi& exchangeApi(trtdata::Exchange) { return api; }
    std::vector<trtdata::Exchange> availableExchanges() { return {trtdata::Exchange::OKX}; }
}

class MonitorTests final : public QObject
{
    Q_OBJECT

private slots:
    void workerSnapshotsAndLifecycle()
    {
        QTest::failOnWarning();
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        api.setPort(server.serverPort());
        connect(&server, &QTcpServer::newConnection, &server, [&server]
        {
            while (server.hasPendingConnections())
            {
                QTcpSocket* socket = server.nextPendingConnection();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, socket, [socket]
                {
                    const QByteArray request = socket->property("request").toByteArray() + socket->readAll();
                    socket->setProperty("request", request);
                    if (!request.contains("\r\n\r\n"))
                        return;
                    const QUrl url(QString::fromUtf8(request.split(' ').value(1)));
                    const bool utc8 = QUrlQuery(url).queryItemValue("zone").toInt() == static_cast<int>(trtdata::TradingDataTimeZone::UTC8);
                    const QByteArray prices = utc8 ? "\"200\",\"240\",\"190\",\"210\"" : "\"100\",\"140\",\"90\",\"110\"";
                    const QByteArray body = "{\"code\":\"0\",\"data\":[[\"1700000000000\"," + prices + ",\"2\",\"3\",\"4\",\"0\"]]}";
                    socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
        trtdata::ConfigManager config(directory.path());
        trtcore::DataMonitor monitor(config);
        const trtdata::InstIdentifier identifier{trtdata::Exchange::OKX, trttrade::InstType::Spot, "BTC-USDT"};
        const trtcore::CandlesManager::SeriesKey key{identifier, trttrade::CandleTimeframe::Minute1};
        monitor.acquireChartSeries(identifier, key.timeframe);
        monitor.acquireChartSeries(identifier, key.timeframe);
        monitor.start();
        monitor.start();
        QTRY_VERIFY_WITH_TIMEOUT(api.hasStream(key), 3000);
        QTRY_VERIFY_WITH_TIMEOUT(monitor.getCandlesManager()->getSnapshot(identifier, key.timeframe) &&
            !monitor.getCandlesManager()->getSnapshot(identifier, key.timeframe)->candles.empty(), 3000);
        QVERIFY(api.getRequestThread() != QThread::currentThread());
        QCOMPARE(api.getRequestThread(), api.getParseThread());
        QCOMPARE(api.getRequestThread(), api.getTickerThread());
        QVERIFY(api.getParserAddress() != &trtdata::parser);
        QTRY_VERIFY(monitor.getMarketData()->getSnapshot(identifier) && monitor.getMarketData()->getSnapshot(identifier)->ticker);
        const std::shared_ptr<const trtcore::MarketDataSnapshot> oldMarket = monitor.getMarketData();
        QCOMPARE(oldMarket->getSnapshot(identifier)->ticker->last, 100.0);
        const std::shared_ptr<const trtcore::CandlesSnapshot> old = monitor.getCandlesManager();
        const trtcore::CandleSnapshot* first = old->getSnapshot(identifier, key.timeframe);
        QCOMPARE(first->candles.back().close, 110.0);
        // Repeated publications reuse unchanged series rather than copying history.
        QTest::qWait(220);
        QCOMPARE(monitor.getCandlesManager()->getSnapshot(identifier, key.timeframe), first);
        QVERIFY(api.dispatch(key, [](MonitorTestCandle* stream) { stream->send(120); }));
        QTRY_COMPARE(monitor.getCandlesManager()->getSnapshot(identifier, key.timeframe)->candles.back().close, 120.0);
        QCOMPARE(first->candles.back().close, 110.0);

        const std::shared_ptr<std::binary_semaphore> entered = std::make_shared<std::binary_semaphore>(0);
        const std::shared_ptr<std::binary_semaphore> resume = std::make_shared<std::binary_semaphore>(0);
        QVERIFY(api.dispatch(key, [entered, resume](MonitorTestCandle*) { entered->release(); resume->acquire(); }));
        const bool blocked = entered->try_acquire_for(std::chrono::seconds(2));
        int ticks = 0;
        QTimer uiTimer;
        connect(&uiTimer, &QTimer::timeout, this, [&ticks] { ticks++; });
        uiTimer.start(5);
        monitor.releaseChartSeries(identifier, key.timeframe);
        const std::shared_ptr<const trtcore::DataSnapshot> whileBusy = monitor.getSnapshot();
        QTest::qWait(60);
        resume->release();
        uiTimer.stop();
        QVERIFY(blocked);
        QVERIFY(ticks > 0);
        QVERIFY(whileBusy->candles.getSnapshot(identifier, key.timeframe));
        QTRY_VERIFY(api.hasStream(key));
        QVERIFY(config.setTradingDataTimeZone(trtdata::TradingDataTimeZone::UTC8));
        QTRY_VERIFY_WITH_TIMEOUT(monitor.getCandlesManager()->getTimeZone() == trtdata::TradingDataTimeZone::UTC8 &&
            monitor.getCandlesManager()->getSnapshot(identifier, key.timeframe) &&
            !monitor.getCandlesManager()->getSnapshot(identifier, key.timeframe)->candles.empty() &&
            monitor.getCandlesManager()->getSnapshot(identifier, key.timeframe)->candles.back().open == 200, 3000);
        QCOMPARE(old->getTimeZone(), trtdata::TradingDataTimeZone::UTC);
        QCOMPARE(first->candles.back().open, 100.0);
        monitor.stop();
        monitor.stop();
        QVERIFY(!api.hasStream(key));
        monitor.start();
        QTRY_VERIFY_WITH_TIMEOUT(api.hasStream(key), 3000);
        monitor.releaseChartSeries(identifier, key.timeframe);
        QTRY_VERIFY(!api.hasStream(key));
        QTRY_VERIFY(!monitor.getCandlesManager()->getSnapshot(identifier, key.timeframe));
        monitor.stop();
        QCOMPARE(oldMarket->getSnapshot(identifier)->ticker->last, 100.0);
        QCOMPARE(first->candles.back().close, 110.0);
    }
};

QTEST_GUILESS_MAIN(MonitorTests)
#include "MonitorTests.moc"
