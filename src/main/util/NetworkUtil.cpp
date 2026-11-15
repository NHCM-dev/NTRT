#include "NetworkUtil.h"
#include <QAbstractEventDispatcher>
#include <QPointer>
#include <QThread>

namespace trtnet
{
    QNetworkAccessManager& getNetworkManager()
    {
        // Replies and their parsing callbacks stay on the requesting thread.
        thread_local QPointer<QNetworkAccessManager> manager;
        if (!manager)
        {
            manager = new QNetworkAccessManager(QAbstractEventDispatcher::instance());
            manager->setTransferTimeout(10000);
            QObject::connect(QThread::currentThread(), &QThread::finished, manager.data(), &QObject::deleteLater);
        }
        return *manager;
    }
}
