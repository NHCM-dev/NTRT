#ifndef NTRT_INSTRUMENTICONMANAGER_H
#define NTRT_INSTRUMENTICONMANAGER_H

#include <QHash>
#include <QIcon>
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
        struct IconEntry
        {
            QIcon icon;
            bool pending = false;
            qint64 nextRequest = 0;
            QPointer<QNetworkReply> reply{};
        };

        QNetworkAccessManager network;
        QHash<QString, IconEntry> icons;
        QTimer requestTimer;
        bool running = false;

        void fetchNext();

        void requestIcon(const QString& symbol, bool crypto);

    public:
        InstrumentIconManager();

        ~InstrumentIconManager() override;

        void setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments);

        void stop();

        QIcon getIcon(const QString& instrumentId) const;
    };
}

#endif //NTRT_INSTRUMENTICONMANAGER_H
