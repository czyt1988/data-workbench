#include "DADataLinkTableModel.h"
// Qt
#include <QColor>
#include <QFont>
// qwt
#include "qwt_plot.h"
#include "qwt_plot_item.h"
#include "qwt_text.h"

namespace DA
{
DADataLinkTableModel::DADataLinkTableModel(QObject* parent) : QStandardItemModel(parent)
{
    setColumnCount(1);
    setHorizontalHeaderItem(0, new QStandardItem(tr("Curve")));  // cn:曲线
}

DADataLinkTableModel::~DADataLinkTableModel()
{
}

QString DADataLinkTableModel::formatXValue(double x)
{
    return QString("x=%1").arg(QString::number(x, 'g', 6));
}

int DADataLinkTableModel::addProbeColumn(double xValue, const QString& headerText, const QString& letter)
{
    int col = columnCount();
    setColumnCount(col + 1);
    // tooltip: formatted x value text for reference
    QString tooltip = headerText.isEmpty() ? formatXValue(xValue) : headerText;
    // header text: probe name if provided, otherwise the x value text
    QString displayText = letter.isEmpty() ? tooltip : letter;
    auto* headerItem    = new QStandardItem(displayText);
    headerItem->setToolTip(tooltip);
    setHorizontalHeaderItem(col, headerItem);
    m_columnXValues.append(xValue);
    return col;
}

void DADataLinkTableModel::removeProbeColumn(int col)
{
    if (col <= 0 || col >= columnCount()) {
        return;
    }
    removeColumn(col);
    int idx = col - 1;
    if (idx >= 0 && idx < m_columnXValues.size()) {
        m_columnXValues.removeAt(idx);
    }
}

void DADataLinkTableModel::renameColumn(int col, const QString& name)
{
    if (col <= 0 || col >= columnCount()) {
        return;
    }
    if (auto* header = horizontalHeaderItem(col)) {
        header->setText(name);
    }
}

QString DADataLinkTableModel::columnLetter(int col) const
{
    if (col <= 0 || col >= columnCount()) {
        return QString();
    }
    auto* header = horizontalHeaderItem(col);
    return header ? header->text() : QString();
}

QStandardItem* DADataLinkTableModel::findPlotRow(QwtPlot* plot) const
{
    for (int r = 0; r < rowCount(); ++r) {
        auto* item = this->item(r, 0);
        if (!item) {
            continue;
        }
        if (!item->data(RoleIsPlotNode).toBool()) {
            continue;
        }
        auto p = reinterpret_cast< QwtPlot* >(item->data(RolePlotPointer).value< quintptr >());
        if (p == plot) {
            return item;
        }
    }
    return nullptr;
}

QStandardItem* DADataLinkTableModel::findCurveRowByTitle(QStandardItem* plotRow, const QString& title) const
{
    if (!plotRow) {
        return nullptr;
    }
    for (int r = 0; r < plotRow->rowCount(); ++r) {
        auto* child = plotRow->child(r, 0);
        if (!child) {
            continue;
        }
        if (child->data(RoleIsPlotNode).toBool()) {
            continue;
        }
        if (child->text() == title) {
            return child;
        }
    }
    return nullptr;
}

QStandardItem* DADataLinkTableModel::ensurePlotRow(QwtPlot* plot, const QString& title)
{
    if (QStandardItem* existing = findPlotRow(plot)) {
        return existing;
    }

    QList< QStandardItem* > rowItems;
    auto* rowItem = new QStandardItem(title);
    rowItem->setData(true, RoleIsPlotNode);
    rowItem->setData(reinterpret_cast< quintptr >(plot), RolePlotPointer);
    rowItem->setEditable(false);
    QFont f = rowItem->font();
    f.setBold(true);
    rowItem->setFont(f);
    rowItems << rowItem;
    for (int c = 1; c < columnCount(); ++c) {
        auto* empty = new QStandardItem();
        empty->setEditable(false);
        rowItems << empty;
    }
    appendRow(rowItems);
    return rowItem;
}

QStandardItem* DADataLinkTableModel::ensureCurveRow(QStandardItem* plotRow, QwtPlotItem* curveItem,
                                                    const QString& title, const QColor& color)
{
    QStandardItem* existing = findCurveRowByTitle(plotRow, title);
    if (existing) {
        existing->setData(reinterpret_cast< quintptr >(curveItem), RolePlotItem);
        existing->setData(color, RoleCurveColor);
        return existing;
    }
    int childRow   = plotRow->rowCount();
    auto* rowItem  = new QStandardItem(title);
    rowItem->setData(false, RoleIsPlotNode);
    rowItem->setData(reinterpret_cast< quintptr >(curveItem), RolePlotItem);
    rowItem->setData(color, RoleCurveColor);
    rowItem->setEditable(false);
    plotRow->setChild(childRow, 0, rowItem);
    for (int c = 1; c < columnCount(); ++c) {
        auto* empty = new QStandardItem();
        empty->setEditable(false);
        plotRow->setChild(childRow, c, empty);
    }
    return rowItem;
}

void DADataLinkTableModel::setCellData(QStandardItem* curveRow, int col, const QString& value)
{
    if (!curveRow || col < 0 || col >= columnCount()) {
        return;
    }
    QStandardItem* parent = curveRow->parent();
    if (!parent) {
        return;
    }
    QStandardItem* cellItem = parent->child(curveRow->row(), col);
    if (!cellItem) {
        cellItem = new QStandardItem(value);
        cellItem->setEditable(false);
        parent->setChild(curveRow->row(), col, cellItem);
    } else {
        cellItem->setText(value);
    }
    auto* curveItem = reinterpret_cast< QwtPlotItem* >(curveRow->data(RolePlotItem).value< quintptr >());
    QColor fg      = (curveItem && !curveItem->isVisible()) ? QColor(150, 150, 150) : QColor(0, 0, 0);
    cellItem->setForeground(fg);
}

void DADataLinkTableModel::removeCurveRow(QwtPlotItem* item)
{
    if (!item) {
        return;
    }
    QString title = item->title().text();
    for (int r = 0; r < rowCount(); ++r) {
        auto* plotItem = this->item(r, 0);
        if (!plotItem) {
            continue;
        }
        if (QStandardItem* row = findCurveRowByTitle(plotItem, title)) {
            plotItem->removeRow(row->row());
            return;
        }
    }
}

void DADataLinkTableModel::updateVisibility()
{
    for (int r = 0; r < rowCount(); ++r) {
        auto* plotItem = this->item(r, 0);
        if (!plotItem) {
            continue;
        }
        for (int c = 0; c < plotItem->rowCount(); ++c) {
            auto* child = plotItem->child(c, 0);
            if (!child) {
                continue;
            }
            auto* curveItem = reinterpret_cast< QwtPlotItem* >(child->data(RolePlotItem).value< quintptr >());
            if (!curveItem) {
                continue;
            }
            QColor fg = curveItem->isVisible() ? QColor(0, 0, 0) : QColor(150, 150, 150);
            child->setForeground(fg);
            for (int col = 1; col < columnCount(); ++col) {
                if (auto* cell = plotItem->child(c, col)) {
                    cell->setForeground(fg);
                }
            }
        }
    }
}

double DADataLinkTableModel::columnXValue(int col) const
{
    int idx = col - 1;
    if (idx >= 0 && idx < m_columnXValues.size()) {
        return m_columnXValues[ idx ];
    }
    return 0.0;
}
}  // namespace DA
