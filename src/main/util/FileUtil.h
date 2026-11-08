//
// Created by NHCM on 2026/11/6.
//

#ifndef NTRT_FILEUTIL_H
#define NTRT_FILEUTIL_H

#include <QFile>

namespace trtutils
{
    bool exists(const QString& path);

    QByteArray readAllBytes(const QString& path);

    QByteArray readAllBytes(const QString& path, QIODeviceBase::OpenMode flags);

    // Overwrites by default. Returns false on open, write, or flush failure.
    bool writeAllBytes(const QString& path, const QByteArray& data);
    bool writeAllBytes(const QString& path, const QByteArray& data, QIODeviceBase::OpenMode flags);

    /**
     * Return relative path, @param path relative path without / as prefix
     */
    QString relativePath(const QString& path);
    QString appendPath(const QString& base, const QString& path);
}

#endif //NTRT_FILEUTIL_H
