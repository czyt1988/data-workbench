#include "DAPySeriesTableView.h"
#include "Models/DAPySeriesTableModel.h"
#include <QHeaderView>
namespace DA
{
DAPySeriesTableView::DAPySeriesTableView(QWidget* parent) : DACacheWindowTableView(parent)
{
}

DAPySeriesTableModel* DAPySeriesTableView::getSeriesModel() const
{
    return qobject_cast< DAPySeriesTableModel* >(model());
}

void DAPySeriesTableView::appendSeries(const DAPySeries& s)
{
    DAPySeriesTableModel* m = getSeriesModel();
    if (!m) {
        return;
    }
    m->appendSeries(s);
    resizeVerticalHeader();
}

void DAPySeriesTableView::appendSeries(const DAAutoincrementSeries< double >& s)
{
    DAPySeriesTableModel* m = getSeriesModel();
    if (!m) {
        return;
    }
    m->appendSeries(s);
    resizeVerticalHeader();
}

void DAPySeriesTableView::insertSeries(int c, const DAPySeries& s)
{
    DAPySeriesTableModel* m = getSeriesModel();
    if (!m) {
        return;
    }
    m->insertSeries(c, s);
    resizeVerticalHeader();
}

void DAPySeriesTableView::insertSeries(int c, const DAAutoincrementSeries< double >& s)
{
    DAPySeriesTableModel* m = getSeriesModel();
    if (!m) {
        return;
    }
    m->insertSeries(c, s);
    resizeVerticalHeader();
}

void DAPySeriesTableView::setSeriesAt(int c, const DAPySeries& s)
{
    DAPySeriesTableModel* m = getSeriesModel();
    if (!m) {
        return;
    }
    m->setSeriesAt(c, s);
    resizeVerticalHeader();
}

void DAPySeriesTableView::setSeriesAt(int c, const DAAutoincrementSeries< double >& s)
{
    DAPySeriesTableModel* m = getSeriesModel();
    if (!m) {
        return;
    }
    m->setSeriesAt(c, s);
    resizeVerticalHeader();
}

void DAPySeriesTableView::resizeVerticalHeader()
{
    DAPySeriesTableModel* m = getSeriesModel();
    if (!m) {
        return;
    }
    int r = m->actualRowCount();
    if (QHeaderView* vh = verticalHeader()) {
        int w = vh->fontMetrics().horizontalAdvance(QString(" %1 ").arg(r));
        vh->setFixedWidth(w);
    }
}
}
