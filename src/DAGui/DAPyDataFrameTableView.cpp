#include "DAPyDataFrameTableView.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QTimer>
#include <QDebug>
#include "Models/DAPyDataFrameTableModel.h"
namespace DA
{
DAPyDataFrameTableView::DAPyDataFrameTableView(QWidget* parent) : QTableView(parent)
{
	setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
	setAlternatingRowColors(false);
	QFontMetrics fm = fontMetrics();
	if (QHeaderView* vheader = verticalHeader()) {
		vheader->setDefaultSectionSize(fm.lineSpacing() * 1.2);
		vheader->setSectionsClickable(false);
		vheader->setSectionResizeMode(QHeaderView::Fixed);
	}
	if (QHeaderView* hheader = horizontalHeader()) {
		hheader->setSectionResizeMode(QHeaderView::Interactive);
	}
	mScrollSignalTimer = new QTimer(this);
	mScrollSignalTimer->setInterval(50);
	connect(mScrollSignalTimer, &QTimer::timeout, this, &DAPyDataFrameTableView::onHandleVerticalScrollBarValueChanged);
	if (QScrollBar* vsc = verticalScrollBar()) {
		connect(vsc, &QScrollBar::valueChanged, this, &DAPyDataFrameTableView::onVerticalScrollBarValueChanged);
	}
}

void DAPyDataFrameTableView::setUndoStack(QUndoStack* stack)
{
	mUndoStack = stack;
}

QUndoStack* DAPyDataFrameTableView::getUndoStack() const
{
	return mUndoStack;
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
}

DAPyDataFrame DAPyDataFrameTableView::getDataframe() const
{
	if (mDataframeModel) {
		return mDataframeModel->dataFrame();
	}
	return DAPyDataFrame();
}

void DAPyDataFrameTableView::onVerticalScrollBarValueChanged(int v)
{
	Q_UNUSED(v);
	mScrollSignalTimer->start();
}

/**
 * @brief 这是处理valuechange的地方，onVerticalScrollBarValueChanged函数重置计时器为了防止抖动导致频繁刷新
 */
void DAPyDataFrameTableView::onHandleVerticalScrollBarValueChanged()
{
	QScrollBar* vsc = verticalScrollBar();
	if (!vsc) {
		return;
	}
	int value = vsc->value();
	int page  = value / mDataframeModel->getPageSize();
	if (page != mDataframeModel->getCurrentPage()) {
		mDataframeModel->setCurrentPage(page);
	}
}
}
