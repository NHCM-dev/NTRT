//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_QTUTIL_H
#define NTRT_QTUTIL_H
#include <QTableWidget>
#include <qwidget.h>

namespace trtutils
{
    QTableWidget* createHeaderTable(QWidget* parent = nullptr, QStringList headers = {});
} // trtutils

#endif //NTRT_QTUTIL_H
