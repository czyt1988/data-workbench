#include "DAPyGridDataTableModel.h"
namespace DA
{
DAPyGridDataTableModel::DAPyGridDataTableModel(QUndoStack* stack, QObject* parent)
    : DAPyDataFrameTableModel(stack, parent)
{
}

DAPyGridDataTableModel::~DAPyGridDataTableModel()
{
}

/**
 * @brief DAPyGridDataTableModel::headerData
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant DAPyGridDataTableModel::actualHeaderData(int actualSection, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	}
	// 剩下都是DisplayRole
	if (Qt::Horizontal == orientation) {  // 说明是水平表头
		if (mXSeries.isNone()) {
			return QVariant();
		}
        if (actualSection >= (int)mXSeries.size()) {
			return QVariant();
		}
        QVariant v = mXSeries[ actualSection ];
		return v;
	} else {  // 垂直表头
		if (mYSeries.isNone()) {
			return QVariant();
		}
        if (actualSection >= (int)mYSeries.size()) {
			return QVariant();
		}
        QVariant v = mYSeries[ actualSection ];
		return v;
	}
	return QVariant();
}

void DAPyGridDataTableModel::setGridX(const DAPySeries& x)
{
	mXSeries = x;
}

void DAPyGridDataTableModel::setGridY(const DAPySeries& y)
{
	mYSeries = y;
}

const DAPySeries& DAPyGridDataTableModel::xSeries() const
{
	return mXSeries;
}

const DAPySeries& DAPyGridDataTableModel::ySeries() const
{
	return mYSeries;
}
}  // end namespace DA
