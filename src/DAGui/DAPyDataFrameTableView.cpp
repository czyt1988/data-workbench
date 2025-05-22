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
	if (QHeaderView* hheader = horizontalHeader()) {
		hheader->setSectionResizeMode(QHeaderView::Interactive);
	}
}

DAPyDataFrameTableView::~DAPyDataFrameTableView()
{
}

void DAPyDataFrameTableView::setDataframeModel(DAPyDataFrameTableModel* dataframeModle)
{
	setModel(dataframeModle);
	mDataframeModel = dataframeModle;
}

DAPyDataFrameTableModel* DAPyDataFrameTableView::getDataframeModel() const
{
	return mDataframeModel;
}

void DAPyDataFrameTableView::setDataFrame(const DAPyDataFrame& d)
{
	if (!mDataframeModel) {
		qWarning() << tr("DataFrameTableView must set model first");  // cn:你需要先设置模型
		return;
	}
	mDataframeModel->setDataFrame(d);
    // 设置数据时，按照行数设置垂直表头的宽度
    if (auto vh = verticalHeader()) {
        auto shape      = d.shape();
        QFontMetrics fm = vh->fontMetrics();
        int w           = fm.horizontalAdvance(QString(" %1 ").arg(shape.first));
        w               = w > 15 ? w : 15;
        vh->setFixedWidth(w);
        // qDebug() << "setFixWidth by" << QString("%1").arg(shape.first) << ",w=" << w;
    }
}

DAPyDataFrame DAPyDataFrameTableView::getDataframe() const
{
	if (mDataframeModel) {
		return mDataframeModel->dataFrame();
	}
	return DAPyDataFrame();
}

}
