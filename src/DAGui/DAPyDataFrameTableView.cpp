#include "DAPyDataFrameTableView.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QDebug>
#include <QElapsedTimer>
#include <QResizeEvent>
#include <QtMath>
#include <QSignalBlocker>
#include "Models/DAPyDataFrameTableModel.h"
namespace DA
{
DAPyDataFrameTableView::DAPyDataFrameTableView(QWidget* parent) : DACacheWindowTableView(parent)
{
	setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
	setAlternatingRowColors(false);
}

DAPyDataFrameTableView::~DAPyDataFrameTableView()
{
}

DAPyDataFrameTableModel* DAPyDataFrameTableView::getDataframeModel() const
{
    return qobject_cast< DAPyDataFrameTableModel* >(model());
}

void DAPyDataFrameTableView::setDataFrame(const DAPyDataFrame& d)
{
    DAPyDataFrameTableModel* m = getDataframeModel();
    if (!m) {
		qWarning() << tr("DataFrameTableView must set model first");  // cn:你需要先设置模型
		return;
	}
    m->setDataFrame(d);
}

DAPyDataFrame DAPyDataFrameTableView::getDataframe() const
{
    DAPyDataFrameTableModel* m = getDataframeModel();
    if (m) {
        return m->dataFrame();
	}
	return DAPyDataFrame();
}

}
