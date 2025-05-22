#include "DACacheWindowTableView.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QElapsedTimer>
#include "Models/DAAbstractCacheWindowTableModel.h"
namespace DA
{
DACacheWindowTableView::DACacheWindowTableView(QWidget* parent) : QTableView(parent)
{
    setAlternatingRowColors(false);
    QFontMetrics fm = fontMetrics();
    if (QHeaderView* vheader = verticalHeader()) {
        vheader->setDefaultSectionSize(fm.lineSpacing() * 1.2);
        vheader->setSectionResizeMode(QHeaderView::Interactive);
    }
    if (QScrollBar* vsc = verticalScrollBar()) {
        connect(vsc, &QScrollBar::valueChanged, this, &DACacheWindowTableView::verticalScrollBarValueChanged);
    }
}

DACacheWindowTableView::~DACacheWindowTableView()
{
}

DAAbstractCacheWindowTableModel* DACacheWindowTableView::getCacheModel() const
{
    return qobject_cast< DAAbstractCacheWindowTableModel* >(model());
}

void DACacheWindowTableView::showActualRow(int actualRow)
{
    // 首先计算行的占比
    DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
    if (!cacheModel) {
        return;
    }
    const int totalRows = cacheModel->getActualRowCount();
    if (actualRow >= totalRows) {
        actualRow = totalRows - 1;
    }
    if (totalRows < 0) {
        actualRow = 0;
    }
    const int maxScroll         = qMax(0, totalRows);
    const double ratio          = static_cast< double >(actualRow) / (totalRows);
    const int targetScrollValue = qMin(static_cast< int >(ratio * maxScroll), maxScroll);
    if (QScrollBar* vsc = verticalScrollBar()) {
        QSignalBlocker b(vsc);
        vsc->setValue(targetScrollValue);
    }
    // 设置startRow
    cacheModel->setCacheWindowStartRow(actualRow);
}

void DACacheWindowTableView::verticalScrollBarValueChanged(int v)
{
    static QElapsedTimer s_elasped;
    if (s_elasped.elapsed() < 50) {
        return;  // 50ms内的重复变化只处理一次,防止抖动
    }
    s_elasped.start();
    DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
    if (!cacheModel) {
        return;
    }
    // 计算滚动比例
    QScrollBar* vsc       = verticalScrollBar();
    const int totalRows   = cacheModel->getActualRowCount();
    const int maxScroll   = qMax(0, totalRows);
    const double ratio    = static_cast< double >(v) / (vsc->maximum() - vsc->minimum());
    const int targetStart = qMin(static_cast< int >(ratio * maxScroll), maxScroll);
    cacheModel->setCacheWindowStartRow(targetStart);
}
}
