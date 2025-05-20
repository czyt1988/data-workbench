#ifndef DAPYDATAFRAMETABLEVIEW_H
#define DAPYDATAFRAMETABLEVIEW_H
#include "DAGuiAPI.h"
#include <QTableView>
#include "pandas/DAPyDataFrame.h"
class QUndoStack;
class QTimer;
class QResizeEvent;
namespace DA
{
class DAPyDataFrameTableModel;

class DAGUI_API DAPyDataFrameTableView : public QTableView
{
	Q_OBJECT
public:
	explicit DAPyDataFrameTableView(QWidget* parent = nullptr);
	~DAPyDataFrameTableView();

	//
	void setDataframeModel(DAPyDataFrameTableModel* dataframeModle);
	DAPyDataFrameTableModel* getDataframeModel() const;
	// 设置datafarme
	void setDataFrame(const DAPyDataFrame& d);
	DAPyDataFrame getDataframe() const;

private Q_SLOTS:
	void onVerticalScrollBarValueChanged(int v);
	void onPageChanged(int page);

protected:
    void resizeEvent(QResizeEvent* event);

private:
	DAPyDataFrameTableModel* mDataframeModel { nullptr };
    int mCurrentShowRows { 20 };  ///< 记录视图当前显示多少行
};
}  // end DA
#endif  // DAPYDATAFRAMETABLEVIEW_H
