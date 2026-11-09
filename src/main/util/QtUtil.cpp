//
// Created by NHCM on 2026/11/8.
//

#include "QtUtil.h"

#include <QHeaderView>

namespace trtutils
{
    QTableWidget* createHeaderTable(QWidget* parent, QStringList headers)
    {
        QTableWidget* table = new QTableWidget(parent);

        table->setFrameShape(QFrame::NoFrame);

        table->setColumnCount(headers.count());
        table->setHorizontalHeaderLabels(headers);

        // Auto adjust column size
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        return table;
    }
} // trtutils