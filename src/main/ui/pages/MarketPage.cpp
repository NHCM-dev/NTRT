//
// Created by NHCM on 2026/11/7.
//

#include "MarketPage.h"
#include "../dialog/InstrumentDialog.h"
#include <QTableWidget>
#include "main/NTRT.h"
#include <QHeaderView>
#include <QLocale>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <cmath>
#include <QPushButton>
#include <QObject>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollBar>
#include <QVBoxLayout>

#include "main/util/QtUtil.h"

namespace trtui
{
    class ExchangeColumnDelegate final : public QStyledItemDelegate
    {
    public:
        explicit ExchangeColumnDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QStyledItemDelegate::paint(painter, option, index);
            const QStyle* style = option.widget ? option.widget->style() : QApplication::style();
            const int gridColor = style->styleHint(QStyle::SH_Table_GridLineColor, &option, option.widget);
            painter->save();
            painter->setPen(QColor::fromRgb(static_cast<QRgb>(gridColor)));
            painter->drawLine(option.rect.topLeft(), option.rect.bottomLeft());
            painter->restore();
        }
    };

    class PriceDelegate final : public QStyledItemDelegate
    {
    public:
        explicit PriceDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

        static void setMovement(QTableWidgetItem* item, const trtcore::MarketSnapshot& snapshot, const QPalette& palette, bool flashEnabled)
        {
            if (!flashEnabled)
                return;

            const trtdata::TickerData& ticker = *snapshot.ticker;
            if (!ticker.previousLast || ticker.last == *ticker.previousLast)
                return;

            const double difference = ticker.last - *ticker.previousLast;
            const bool dark = palette.color(QPalette::Base).lightness() < 128;
            const QColor color = difference > 0 ? QColor(dark ? 0x4ade80 : 0x15803d) : QColor(dark ? 0xfb7185 : 0xb91c1c);
            if (snapshot.quoteAge.isValid() && snapshot.quoteAge.elapsed() < 350)
            {
                item->setForeground(color);
                return;
            }

            const QString price = item->text();
            const qsizetype start = changedDigitOffset(price, std::abs(difference));
            if (start < price.size())
            {
                item->setData(Qt::UserRole, start);
                item->setData(Qt::UserRole + 1, color);
            }
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            const QVariant start = index.data(Qt::UserRole);
            QStyleOptionViewItem styled(option);
            initStyleOption(&styled, index);
            QStyle* style = styled.widget ? styled.widget->style() : QApplication::style();
            const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &styled, styled.widget) + 1;
            const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &styled, styled.widget).adjusted(margin, 0, -margin, 0);
            const QString price = styled.fontMetrics.elidedText(styled.text, styled.textElideMode, textRect.width());
            if (price.isEmpty())
            {
                QStyledItemDelegate::paint(painter, option, index);
                return;
            }
            styled.text.clear();
            style->drawControl(QStyle::CE_ItemViewItem, &styled, painter, styled.widget);

            QTextLayout text(price, styled.font);
            if (start.isValid() && start.toInt() < price.size())
            {
                QTextLayout::FormatRange suffix;
                suffix.start = start.toInt();
                suffix.length = static_cast<int>(price.size()) - suffix.start;
                suffix.format.setForeground(index.data(Qt::UserRole + 1).value<QColor>());
                text.setFormats({suffix});
            }
            text.beginLayout();
            QTextLine line = text.createLine();
            line.setLineWidth(textRect.width());
            text.endLayout();

            painter->save();
            painter->setClipRect(textRect, Qt::IntersectClip);
            painter->setPen(styled.palette.color(QPalette::Text));
            const QSize textSize(static_cast<int>(std::ceil(line.naturalTextWidth())), static_cast<int>(std::ceil(line.height())));
            const QRect aligned = QStyle::alignedRect(styled.direction, styled.displayAlignment, textSize, textRect);
            text.draw(painter, aligned.topLeft());
            painter->restore();
        }

    private:
        static qsizetype changedDigitOffset(const QString& price, double difference)
        {
            const int changedPower = static_cast<int>(std::floor(std::log10(difference) + 1e-9));
            const qsizetype exponentPosition = price.indexOf('e', 0, Qt::CaseInsensitive);
            const qsizetype end = exponentPosition < 0 ? price.size() : exponentPosition;
            const int exponent = exponentPosition < 0 ? 0 : price.mid(exponentPosition + 1).toInt();
            const qsizetype decimal = price.indexOf('.');
            const qsizetype integerEnd = decimal < 0 ? end : decimal;
            int power = exponent - 1;
            for (qsizetype index = 0; index < integerEnd; index++)
            {
                if (price[index].isDigit())
                    power++;
            }
            for (qsizetype index = 0; index < end; index++)
            {
                if (!price[index].isDigit())
                    continue;
                if (power <= changedPower)
                    return index;
                power--;
            }
            return price.size();
        }
    };

    QFrame* MarketPage::generateWidget(QWidget* parent)
    {
        QFrame* page = new QFrame(parent);

        page->setBackgroundRole(QPalette::Base);
        page->setAutoFillBackground(true);

        QVBoxLayout* layout = new QVBoxLayout(page);
        QHBoxLayout* filters = new QHBoxLayout;
        search = new QLineEdit(page);
        search->setPlaceholderText("Search instruments");
        typeFilter = new QComboBox(page);
        typeFilter->addItem("All types", -1);
        for (trtdata::InstType type : {trtdata::InstType::Spot, trtdata::InstType::Margin, trtdata::InstType::Swap,
                                     trtdata::InstType::Futures, trtdata::InstType::Option, trtdata::InstType::Events})
            typeFilter->addItem(QString::fromUtf8(trtdata::instrumentTypeName(type)), static_cast<int>(type));
        filters->addWidget(search, 1);
        filters->addWidget(typeFilter);
        layout->addLayout(filters);

        table = trtutils::createHeaderTable(page, {"Exchange", "Type", "Instrument", "Last Price", "24H Volume", "24H Change"});
        table->setItemDelegateForColumn(0, new ExchangeColumnDelegate(table));
        table->setItemDelegateForColumn(3, new PriceDelegate(table));

        QHeaderView* header = table->horizontalHeader();
        header->setSectionResizeMode(0, QHeaderView::Interactive);
        header->setSectionResizeMode(3, QHeaderView::Interactive);
        header->setSectionResizeMode(4, QHeaderView::Interactive);
        header->setSectionResizeMode(5, QHeaderView::Interactive);
        header->setSectionResizeMode(1, QHeaderView::Fixed);

        header->setMinimumSectionSize(100);
        header->setSectionResizeMode(2, QHeaderView::Stretch);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        table->setColumnWidth(0, 100);
        table->setColumnWidth(1, 100);
        table->setColumnWidth(3, 110);
        table->setColumnWidth(4, 130);
        table->setColumnWidth(5, 100);
        table->verticalHeader()->hide();
        int minimumWidth = header->minimumSectionSize() + table->verticalScrollBar()->sizeHint().width();
        for (int column = 0; column < header->count(); column++)
        {
            if (header->sectionResizeMode(column) != QHeaderView::Stretch)
                minimumWidth += header->sectionSize(column);
        }
        table->setMinimumWidth(minimumWidth + 2 * table->frameWidth());
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(table, 1);

        QHBoxLayout* actions = new QHBoxLayout;
        actions->addStretch();
        QPushButton* manageButton = new QPushButton("Manage", page);
        QPushButton* addButton = new QPushButton("Add", page);
        actions->addWidget(manageButton);
        actions->addWidget(addButton);
        layout->addLayout(actions);

        QObject::connect(search, &QLineEdit::textChanged, page, [this] { update(); });
        QObject::connect(typeFilter, &QComboBox::currentIndexChanged, page, [this] { update(); });
        QObject::connect(addButton, &QPushButton::clicked, page, [this, page]
        {
            InstrumentDialog dialog(InstrumentDialog::Mode::Add, page);
            if (dialog.exec() == QDialog::Accepted)
                update();
        });
        QObject::connect(manageButton, &QPushButton::clicked, page, [this, page]
        {
            InstrumentDialog dialog(InstrumentDialog::Mode::Manage, page);
            if (dialog.exec() == QDialog::Accepted)
                update();
        });

        return page;
    }

    void MarketPage::update()
    {
        const std::span<const std::shared_ptr<trtdata::Instrument>> instruments = ntrt::configManager->getInstruments();
        const trtcore::MarketData& marketData = ntrt::dataMonitor->getMarketData();
        const QString query = search->text().trimmed();
        const int filter = typeFilter->currentData().toInt();
        const QLocale locale(QLocale::English);
        const bool flashEnabled = ntrt::configManager->getAppearanceManager()->isPriceFlashEnabled();
        int row = 0;
        for (const std::shared_ptr<trtdata::Instrument>& instrument : instruments)
        {
            const QString exchange = QString::fromUtf8(trtdata::getExchangeStr(instrument->exchange));
            const QString symbol = QString::fromStdString(instrument->instId);
            if ((filter >= 0 && filter != static_cast<int>(instrument->instType)) ||
                (!symbol.contains(query, Qt::CaseInsensitive) && !exchange.contains(query, Qt::CaseInsensitive)))
                continue;

            if (row == table->rowCount())
                table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(exchange));
            table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(trtdata::instrumentTypeName(instrument->instType))));
            table->setItem(row, 2, new QTableWidgetItem(symbol));

            const trtcore::MarketSnapshot* snapshot = marketData.getSnapshot(instrument->getUniqueIdentifier());
            if (!snapshot || !snapshot->ticker)
            {
                QTableWidgetItem* status = new QTableWidgetItem("No data");
                if (snapshot && !snapshot->error.isEmpty())
                {
                    status->setText("Request failed");
                    status->setToolTip(snapshot->error);
                }
                table->setItem(row, 3, status);
                table->setItem(row, 4, new QTableWidgetItem("-"));
                QTableWidgetItem* change = new QTableWidgetItem("-");
                change->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                table->setItem(row, 5, change);
                row++;
                continue;
            }

            const trtdata::TickerData& data = *snapshot->ticker;
            const QString price = data.last >= 1 ? locale.toString(data.last, 'f', 2) : locale.toString(data.last, 'g', 4);
            QTableWidgetItem* priceItem = new QTableWidgetItem(price);
            PriceDelegate::setMovement(priceItem, *snapshot, table->palette(), flashEnabled);
            if (!snapshot->error.isEmpty())
                priceItem->setToolTip("Last successful quote: " + snapshot->error);
            table->setItem(row, 3, priceItem);

            const QString instId = QString::fromStdString(data.instId);
            const bool spot = data.instType == trtdata::InstType::Spot || data.instType == trtdata::InstType::Margin;
            const double baseVol = spot ? data.vol24h : data.volCcy24h;
            const double quoteVol = spot ? data.volCcy24h : data.volCcy24h * data.last;
            QTableWidgetItem* volume = new QTableWidgetItem(locale.toString(baseVol, 'f', 2) + " " + instId.section('-', 0, 0));
            volume->setToolTip(locale.toString(quoteVol, 'f', 2) + " " + instId.section('-', 1, 1));
            table->setItem(row, 4, volume);

            QTableWidgetItem* change = new QTableWidgetItem("N/A");
            if (data.open24h != 0)
            {
                const double percent = (data.last - data.open24h) / data.open24h * 100.0;
                const QString text = QString::number(percent, 'f', 2);
                if (text == "0.00" || text == "-0.00")
                    change->setText("0.00%");
                else
                {
                    change->setText((percent > 0 ? "+" : "") + text + "%");
                    change->setForeground(percent >= 0 ? QColor(0x15803d) : QColor(0xb91c1c));
                }
                change->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            table->setItem(row, 5, change);
            row++;
        }
        table->setRowCount(row);
    }
} // trtui
