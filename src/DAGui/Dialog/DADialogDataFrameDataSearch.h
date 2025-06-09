#ifndef DADIALOGDATAFRAMEDATASEARCH_H
#define DADIALOGDATAFRAMEDATASEARCH_H

#include <QDialog>
#include "pandas/DAPyDataFrame.h"
namespace Ui
{
class DADialogDataFrameDataSearch;
}

namespace DA
{
class DAPyDataFrameTableView;

class DADialogDataFrameDataSearch : public QDialog
{
	Q_OBJECT

public:
	explicit DADialogDataFrameDataSearch(QWidget* parent = nullptr);
	~DADialogDataFrameDataSearch();
	// 获取查找内容
	QString getSearchText() const;
	// 获取内容坐标
	QList< QPair< int, int > > getItemCoor() const;

	DAPyDataFrameTableView* getDataframeTableView() const;
	void setDataframeTableView(DAPyDataFrameTableView* v);
	// 搜索
	void searchData();
private slots:
	void onPushButtonNextClicked();
	void onLineEditTextChanged(const QString& t);

private:
	Ui::DADialogDataFrameDataSearch* ui;
	DAPyDataFrameTableView* mDataframeTableView { nullptr };
	QList< QPair< int, int > > mMatches {};
	bool mIsNeedResearch { true };  ///< 需要重新搜索，这个在重新设置了dataframe后触发
	int mIndex { -1 };              ///< -1代表全新的搜索，需要重新匹配一下mMatches
};
}

#endif  // DADIALOGDATAFRAMEDATASEARCH_H
