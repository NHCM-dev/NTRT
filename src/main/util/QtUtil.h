//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_QTUTIL_H
#define NTRT_QTUTIL_H
#include <QTableWidget>
#include <QWheelEvent>
#include <qwidget.h>

namespace trtutils
{
    double wheelZoomFactor(const QWheelEvent& event);

    bool isWheelPan(const QWheelEvent& event);

    double wheelPanPixels(const QWheelEvent& event);

    QRect boundedWidgetGeometry(const QRect& geometry, const QRect& bounds);

    QRect resizedWidgetGeometry(const QRect& geometry, const QPoint& delta, Qt::Edges edges,
                                const QRect& bounds, const QSize& minimum);

    void updateWidgetTreeRegion(QWidget* widget, const QRegion& region);

    QTableWidgetItem* updateTableItem(QTableWidget* table, int row, int column, const QString& text);

    void swapTableRows(QTableWidget* table, int first, int second);

    void centerOnScreen(QWidget * widget);

    void fitWindowToScreen(QWidget* widget);
    QTableWidget* createHeaderTable(QWidget * parent = nullptr, QStringList headers = {});
} // trtutils

#endif //NTRT_QTUTIL_H
