#ifndef NTRT_INSTRUMENTICONMANAGER_H
#define NTRT_INSTRUMENTICONMANAGER_H

#include <QHash>
#include <QImage>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTimer>
#include "main/market/abstract/Instrument.h"
#include <memory>
#include <span>

namespace trtcore
{
    class InstrumentIconManager final : public QObject
    {
        Q_OBJECT
        struct IconEntry
        {
            bool pending = false;
            qint64 nextRequest = 0;
            QPointer<QNetworkReply> reply{};
        };

        QNetworkAccessManager network;
        QHash<QString, IconEntry> icons;
        QHash<QString, QImage> images;
        QTimer requestTimer;
        bool running = false;

        void fetchNext();

        void requestIcon(const QString& symbol, bool crypto);

    public:
        InstrumentIconManager();

        ~InstrumentIconManager() override;

        void setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments);

        void stop();

        const QHash<QString, QImage>& getImages() const;
    signals:
        void snapshotChanged();
    };
}

#endif //NTRT_INSTRUMENTICONMANAGER_H
