//
// Created by NHCM on 2026/11/6.
//

#ifndef NTRT_FILEUTIL_H
#define NTRT_FILEUTIL_H

#include <QFile>

namespace trtutils
{
    QByteArray readAllBytes(const char* path);

    QByteArray readAllBytes(const char* path, QIODeviceBase::OpenMode flags);

    /**
     * Return relative path, @param path relative path without / as prefix
     */
    QString relativePath(const QString& path);
}

#endif //NTRT_FILEUTIL_H
