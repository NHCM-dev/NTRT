//
// Created by NHCM on 2026/11/6.
//

#ifndef NTRT_FILEUTILS_H
#define NTRT_FILEUTILS_H

#include <QFile>

namespace trtutils
{
    QByteArray readAllBytes(const char* path);

    QByteArray readAllBytes(const char* path, QIODeviceBase::OpenMode flags);
}

#endif //NTRT_FILEUTILS_H
