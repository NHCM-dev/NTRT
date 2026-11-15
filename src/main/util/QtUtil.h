//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_QTUTIL_H
#define NTRT_QTUTIL_H
#include <QTableWidget>
#include <qwidget.h>

namespace trtutils
{
    QTableWidgetItem* updateTableItem(QTableWidget* table, int row, int column, const QString& text);

    void swapTableRows(QTableWidget* table, int first, int second);

    void centerOnScreen(QWidget * widget);
    QTableWidget* createHeaderTable(QWidget * parent = nullptr, QStringList headers = {});
} // trtutils

#endif //NTRT_QTUTIL_H
