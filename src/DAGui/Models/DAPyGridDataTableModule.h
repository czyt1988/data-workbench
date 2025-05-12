#ifndef DAPYGRIDDATATABLEMODULE_H
#define DAPYGRIDDATATABLEMODULE_H
#include "DAPyDataFrameTableModule.h"
namespace DA
{
/**
 * @brief 这个类专门针对网格数据显示，用于QwtGridRasterData数据,会定义一个x序列，定义一个y序列，和值
 */
class DAGUI_API DAPyGridDataTableModule : public DAPyDataFrameTableModule
{
public:
	DAPyGridDataTableModule(QUndoStack* stack, QObject* parent = nullptr);
	~DAPyGridDataTableModule();
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

#endif  // DAPYGRIDDATATABLEMODULE_H
