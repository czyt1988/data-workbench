#include "DAPyGridDataTableModule.h"
namespace DA
{
DAPyGridDataTableModule::DAPyGridDataTableModule(QUndoStack* stack, QObject* parent)
    : DAPyDataFrameTableModule(stack, parent)
{
}

DAPyGridDataTableModule::~DAPyGridDataTableModule()
{
}

/**
 * @brief DAPyGridDataTableModule::headerData
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant DAPyGridDataTableModule::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	}
	// 剩下都是DisplayRole
	if (Qt::Horizontal == orientation) {  // 说明是水平表头
		if (mXSeries.isNone()) {
			return QVariant();
		}
		if (section >= (int)mXSeries.size()) {
			return QVariant();
		}
		QVariant v = mXSeries[ section ];
		return v;
	} else {  // 垂直表头
		if (mYSeries.isNone()) {
			return QVariant();
		}
		if (section >= (int)mYSeries.size()) {
			return QVariant();
		}
		QVariant v = mYSeries[ section ];
		return v;
	}
	return QVariant();
}

void DAPyGridDataTableModule::setGridX(const DAPySeries& x)
{
	mXSeries = x;
}

void DAPyGridDataTableModule::setGridY(const DAPySeries& y)
{
	mYSeries = y;
}

const DAPySeries& DAPyGridDataTableModule::xSeries() const
{
	return mXSeries;
}

const DAPySeries& DAPyGridDataTableModule::ySeries() const
{
	return mYSeries;
}
}  // end namespace DA
