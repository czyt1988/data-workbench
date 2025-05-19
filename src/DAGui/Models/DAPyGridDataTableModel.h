#ifndef DAPYGRIDDATATABLEMODEL_H
#define DAPYGRIDDATATABLEMODEL_H
#include "DAPyDataFrameTableModel.h"
namespace DA
{
/**
 * @brief 这个类专门针对网格数据显示，用于QwtGridRasterData数据,会定义一个x序列，定义一个y序列，和值
 */
class DAGUI_API DAPyGridDataTableModel : public DAPyDataFrameTableModel
{
public:
	DAPyGridDataTableModel(QUndoStack* stack, QObject* parent = nullptr);
	~DAPyGridDataTableModel();
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public:
	void setGridX(const DAPySeries& x);
	void setGridY(const DAPySeries& y);

	const DAPySeries& xSeries() const;

	const DAPySeries& ySeries() const;

private:
	DAPySeries mXSeries;
	DAPySeries mYSeries;
};

}

#endif  // DAPYGRIDDATATABLEMODEL_H
