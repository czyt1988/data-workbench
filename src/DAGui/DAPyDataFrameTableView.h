#ifndef DAPYDATAFRAMETABLEVIEW_H
#define DAPYDATAFRAMETABLEVIEW_H
#include "DAGuiAPI.h"
#include "DACacheWindowTableView.h"
#include "pandas/DAPyDataFrame.h"
namespace DA
{
class DAPyDataFrameTableModel;

class DAGUI_API DAPyDataFrameTableView : public DACacheWindowTableView
{
	Q_OBJECT
public:
	explicit DAPyDataFrameTableView(QWidget* parent = nullptr);
	~DAPyDataFrameTableView();
	DAPyDataFrameTableModel* getDataframeModel() const;
	// 设置datafarme
	void setDataFrame(const DAPyDataFrame& d);
	DAPyDataFrame getDataframe() const;
};
}  // end DA
#endif  // DAPYDATAFRAMETABLEVIEW_H
