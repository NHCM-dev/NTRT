//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_NETWORKUTIL_H
#define NTRT_NETWORKUTIL_H

#include <QNetworkReply>
#include <QPromise>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace trtnet
{
    // Takes responsibility for deleting the reply after completion.
    // Call on the reply's thread. The handler receives the response body there.
    template<class Handler>
    QFuture<std::remove_cvref_t<std::invoke_result_t<Handler&, const QByteArray&>>> replyToFuture(QNetworkReply* reply, Handler&& onSuccess)
    {
        using Result = std::remove_cvref_t<std::invoke_result_t<Handler&, const QByteArray&>>;
        std::shared_ptr<QPromise<Result>> promise = std::make_shared<QPromise<Result>>();
        promise->start();
        QFuture<Result> future = promise->future();

        if (!reply)
        {
            promise->setException(std::make_exception_ptr(std::runtime_error("Failed to create network request")));
            promise->finish();
            return future;
        }

        std::move_only_function<void()> complete = [reply, promise, handler = std::forward<Handler>(onSuccess)]() mutable
        {
            try
            {
                if (reply->error() != QNetworkReply::NoError)
                    throw std::runtime_error(reply->errorString().toStdString());

                const QByteArray body = reply->readAll();
                if constexpr (std::is_void_v<Result>)
                    std::invoke(handler, body);
                else
                    promise->addResult(std::invoke(handler, body));
            }
            catch (...)
            {
                promise->setException(std::current_exception());
            }

            reply->deleteLater();
            promise->finish();
        };

        if (reply->isFinished())
            complete();
        else
            QObject::connect(reply, &QNetworkReply::finished, reply, std::move(complete), Qt::SingleShotConnection);

        return future;
    }
} // trtnet

#endif //NTRT_NETWORKUTIL_H
