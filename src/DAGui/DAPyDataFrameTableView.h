#ifndef DAPYDATAFRAMETABLEVIEW_H
#define DAPYDATAFRAMETABLEVIEW_H
#include "DAGuiAPI.h"
#include <QTableView>
#include "pandas/DAPyDataFrame.h"
class QUndoStack;
class QTimer;
namespace DA
{
class DAPyDataFrameTableModel;

class DAGUI_API DAPyDataFrameTableView : public QTableView
{
	Q_OBJECT
public:
	explicit DAPyDataFrameTableView(QWidget* parent = nullptr);
	~DAPyDataFrameTableView();
	// undo
	void setUndoStack(QUndoStack* stack);
	QUndoStack* getUndoStack() const;
	//
	void setDataframeModel(DAPyDataFrameTableModel* dataframeModle);
	DAPyDataFrameTableModel* getDataframeModel() const;
	// 设置datafarme
	void setDataFrame(const DAPyDataFrame& d);
	DAPyDataFrame getDataframe() const;
private Q_SLOTS:
	void onVerticalScrollBarValueChanged(int v);
	void onHandleVerticalScrollBarValueChanged();

private:
	QUndoStack* mUndoStack { nullptr };
	QTimer* mScrollSignalTimer { nullptr };  ///< 防止快速拉动滚动条导致频繁刷新用的一个延迟信号发送器
	DAPyDataFrameTableModel* mDataframeModel { nullptr };
};
}  // end DA
#endif  // DAPYDATAFRAMETABLEVIEW_H
