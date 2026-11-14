#include "InstrumentIconManager.h"
#include <QDateTime>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QPainter>
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
        connect(&requestTimer, &QTimer::timeout, this, [this] { fetchNext(); });
    }

    InstrumentIconManager::~InstrumentIconManager()
    {
        stop();
    }

    QIcon InstrumentIconManager::createFallback(const QString& symbol) const
    {
        QPixmap image(40, 40);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0x53647c));
        painter.drawEllipse(0, 0, 40, 40);
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPixelSize(20);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(image.rect(), Qt::AlignCenter, symbol.left(1));
        return QIcon(image);
    }

    void InstrumentIconManager::setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments)
    {
        QSet<QString> desired;
        for (const std::shared_ptr<trtdata::Instrument>& instrument : instruments)
        {
            const QString symbol = QString::fromStdString(instrument->instId).section('-', 0, 0).toUpper();
            desired.insert(symbol);
            if (!icons.contains(symbol))
                icons.insert(symbol, IconEntry{createFallback(symbol)});
        }
        for (QHash<QString, IconEntry>::iterator entry = icons.begin(); entry != icons.end();)
        {
            if (desired.contains(entry.key()))
            {
                entry++;
                continue;
            }
            QPointer<QNetworkReply> reply = entry->reply;
            entry = icons.erase(entry);
            if (reply)
                reply->abort();
        }
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

    QIcon InstrumentIconManager::getIcon(const QString& instrumentId) const
    {
        const QString symbol = instrumentId.section('-', 0, 0).toUpper();
        const QHash<QString, IconEntry>::const_iterator entry = icons.constFind(symbol);
        return entry == icons.cend() ? QIcon() : entry->icon;
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
        QNetworkRequest request(QUrl("https://logoapi.sh/" + route + QString::fromLatin1(QUrl::toPercentEncoding(symbol)) + ".png?size=128&fallback=404"));
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        QNetworkReply* reply = network.get(request);
        icons[symbol].reply = reply;
        connect(reply, &QNetworkReply::finished, this, [this, symbol, crypto, reply]
        {
            reply->deleteLater();
            if (!running || !icons.contains(symbol) || icons[symbol].reply != reply)
                return;
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QPixmap image;
            const bool loaded = reply->error() == QNetworkReply::NoError && image.loadFromData(reply->readAll());
            IconEntry& entry = icons[symbol];
            if (loaded)
                entry.icon = QIcon(image);
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
