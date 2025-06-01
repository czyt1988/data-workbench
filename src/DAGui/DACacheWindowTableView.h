#ifndef DACACHEWINDOWTABLEVIEW_H
#define DACACHEWINDOWTABLEVIEW_H
#include "DAGuiAPI.h"
#include <QTableView>
#include <QTableWidget>
namespace DA
{
class DAAbstractCacheWindowTableModel;
/**
 * @brief 针对DAAbstractCacheWindowTableModel的view
 */
class DAGUI_API DACacheWindowTableView : public QTableView
{
	Q_OBJECT
public:
	DACacheWindowTableView(QWidget* parent = nullptr);
	~DACacheWindowTableView();
	// 获取缓存窗口model
	DAAbstractCacheWindowTableModel* getCacheModel() const;
	// 显示行
	void showActualRow(int actualRow);
	// 高亮目标文本框
	void selectActualCell(int row, int col);
	// 真实行转换为逻辑行
	int toLogicalRow(int actualRow) const;
	// 判断当前的真实行是否再视图的可见范围内
	bool isActualRowInViewRange(int actualRow) const;
	// 真实行的名称
	QString actualRowName(int actualRow) const;
	// 真实列的名称
	QString actualColumnName(int actualCol) const;
private Q_SLOTS:
	virtual void verticalScrollBarValueChanged(int v);
};
}

#endif  // DACACHEWINDOWTABLEVIEW_H
