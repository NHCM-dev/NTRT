#include "InstrumentIconManager.h"
#include <QDateTime>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QSet>
#include "main/util/FileUtil.h"

namespace trtcore
{
    InstrumentIconManager::InstrumentIconManager()
    {
        network.setTransferTimeout(10000);
        QNetworkDiskCache* cache = new QNetworkDiskCache(&network);
        cache->setCacheDirectory(trtutils::relativePath("app/cache/instrument-icons"));
        cache->setMaximumCacheSize(16 * 1024 * 1024);
        network.setCache(cache);
        requestTimer.setInterval(250);
        connect(&requestTimer, &QTimer::timeout, this, [this]
        {
            fetchNext();
        });
    }

    InstrumentIconManager::~InstrumentIconManager()
    {
        stop();
    }

    void InstrumentIconManager::setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments)
    {
        QSet<QString> desired;
        for (const std::shared_ptr<trtdata::Instrument>& instrument : instruments)
        {
            const QString symbol = QString::fromStdString(instrument->instId).section('-', 0, 0).toUpper();
            desired.insert(symbol);
            if (!icons.contains(symbol))
                icons.insert(symbol, IconEntry{});
        }
        bool imagesRemoved = false;
        for (QHash<QString, IconEntry>::iterator entry = icons.begin(); entry != icons.end();)
        {
            if (desired.contains(entry.key()))
            {
                entry++;
                continue;
            }
            QPointer<QNetworkReply> reply = entry->reply;
            imagesRemoved |= images.remove(entry.key());
            entry = icons.erase(entry);
            if (reply)
                reply->abort();
        }
        if (imagesRemoved) emit snapshotChanged();
        running = !icons.empty();
        if (running)
            requestTimer.start();
        else
            requestTimer.stop();
    }

    void InstrumentIconManager::stop()
    {
        running = false;
        requestTimer.stop();
        for (IconEntry& entry : icons)
        {
            if (entry.reply)
                entry.reply->abort();
            entry.reply = nullptr;
            entry.pending = false;
        }
    }

    const QHash<QString, QImage>& InstrumentIconManager::getImages() const
    {
        return images;
    }

    void InstrumentIconManager::fetchNext()
    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        int pending = 0;
        QHash<QString, IconEntry>::iterator next = icons.end();
        for (QHash<QString, IconEntry>::iterator entry = icons.begin(); entry != icons.end(); entry++)
        {
            if (entry->pending)
                pending++;
            else if (next == icons.end() && entry->nextRequest <= now)
                next = entry;
        }
        if (pending < 4 && next != icons.end())
        {
            next->pending = true;
            requestIcon(next.key(), true);
        }
    }

    void InstrumentIconManager::requestIcon(const QString& symbol, bool crypto)
    {
        const QString route = crypto ? "crypto/" : "icon/ticker/";
        QNetworkRequest request(QUrl(
            "https://logoapi.sh/" + route + QString::fromLatin1(QUrl::toPercentEncoding(symbol)) +
            ".png?size=128&fallback=404"));
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        QNetworkReply* reply = network.get(request);
        icons[symbol].reply = reply;
        connect(reply, &QNetworkReply::finished, this, [this, symbol, crypto, reply]
        {
            reply->deleteLater();
            if (!running || !icons.contains(symbol) || icons[symbol].reply != reply)
                return;
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QImage image;
            const bool loaded = reply->error() == QNetworkReply::NoError && image.loadFromData(reply->readAll());
            IconEntry& entry = icons[symbol];
            if (loaded)
            {
                images.insert(symbol, std::move(image));
                emit snapshotChanged();
            }
            if (!loaded && crypto && status == 404)
            {
                requestIcon(symbol, false);
                return;
            }
            entry.pending = false;
            entry.reply = nullptr;
            entry.nextRequest = QDateTime::currentMSecsSinceEpoch() + (loaded ? 86400000 : 300000);
        });
    }
}
