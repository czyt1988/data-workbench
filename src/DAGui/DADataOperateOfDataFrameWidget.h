#ifndef DADATAOPERATEOFDATAFRAMEWIDGET_H
#define DADATAOPERATEOFDATAFRAMEWIDGET_H
#include <QtWidgets/QWidget>
#include <QUndoStack>
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataPyDataFrame.h"
#include "numpy/DAPyDType.h"
#include "DADataOperatePageWidget.h"
namespace Ui
{
class DADataOperateOfDataFrameWidget;
}

namespace DA
{
class DAPyDataFrameTableModule;
class DAAppRibbonArea;
class DADialogDataframeColumnCastToNumeric;
class DADialogDataframeColumnCastToDatetime;
class DADialogDataFrameFillna;
class DADialogDataFrameFillInterpolate;
class DADialogDataFrameClipOutlier;

/**
 * @brief 针对DataFrame的操作窗口
 */
class DAGUI_API DADataOperateOfDataFrameWidget : public DADataOperatePageWidget
{
	Q_OBJECT
public:
	virtual int getDataOperatePageType() const;

public:
	explicit DADataOperateOfDataFrameWidget(const DAData& d, QWidget* parent = nullptr);
	~DADataOperateOfDataFrameWidget();
	// 是否存在data
	bool haveData() const;
	// 获取dataframe
	DAPyDataFrame getDataframe() const;
	// 获取Data的引用
	const DAData& data() const;
	// 创建dataframe的数据描述
	DAPyDataFrame createDataDescribe();
	// dataframe表格是否有选中项
	bool isDataframeTableHaveSelection() const;
	// 返回选中的列数，列数不会重复
	QList< int > getSelectedDataframeCoumns(bool ensureInDataframe = true) const;
	QList< int > getSelectedDataframeRows(bool ensureInDataframe = true) const;
	// 返回选中的列/行
	QList< int > getFullySelectedDataframeColumns(bool ensureInDataframe = true) const;
	QList< int > getFullySelectedDataframeRows(bool ensureInDataframe = true) const;

	QList< QPoint > getSelectedDataframeCells(bool ensureInDataframe = true) const;
	int getSelectedOneDataframeRow(bool ensureInDataframe = true) const;
	int getSelectedOneDataframeColumn(bool ensureInDataframe = true) const;

	// 获取选中的序列，如果用户打开一个表格，选中了其中一列，那么将返回那一列pd.Series作为数据，如果用户选中了多列，那么每列作为一个DAData并组成list返回
	QList< DAData > getSlectedSeries() const;
public Q_SLOTS:
	void setDAData(const DA::DAData& d);
	// 在选中行后面插入行
	void insertRowAboveBySelect();
	void insertRowBelowBySelect();
	void insertRowAt(int row);
	void insertColumnRightBySelect();
	void insertColumnLeftBySelect();
	void insertColumnAt(int col);
	// 移除选中的行
	int removeSelectRow();
	// 移除选择的列
	int removeSelectColumn();
	// 设置选中单元格为nan,返回设置成功的个数
	int removeSelectCell();

	// 列更名
	void renameColumns();
	// 设置选择列的数据类型，发射信号columnTypeChanged
	bool changeSelectColumnType(const DAPyDType& dt);
	// 把选择的行转换为数值，带交互
	void castSelectToNum();
	void castSelectToDatetime();
	// 把选择的列转换为索引
	bool changeSelectColumnToIndex();
	// 删除缺失值,返回删除的数量
	int dropna(const QString& how = QStringLiteral("any"), int thresh = -1);
	int dropna(const DAPyDataFrame& df,
               int axis,
               const QString& how       = QStringLiteral("any"),
               const QList< int > index = QList< int >(),
               int thresh               = -1);
	// 删除重复值
	int dropduplicates(const QString& keep = QStringLiteral("first"));
	int dropduplicates(const DAPyDataFrame& df,
                       const QString& keep      = QStringLiteral("first"),
                       const QList< int > index = QList< int >());

	// 填充缺失值，执行成功返回true
	bool fillna();
	bool fillna(const DAPyDataFrame& df, double value = 0.0, int limit = -1);
	// 插值法填充缺失值，成功返回true
    bool interpolate();
    bool interpolate(const DAPyDataFrame& df, const QString& method, int order, int limit);
	// 前向填充缺失值，执行成功返回true
	bool ffillna();
	bool ffillna(const DAPyDataFrame& df, int axis = 0, int limit = -1);
	// 后向填充缺失值，执行成功返回true
	bool bfillna();
	bool bfillna(const DAPyDataFrame& df, int axis = 0, int limit = -1);
	// n倍标准差过滤异常值
	int nstdfilteroutlier(double n = 3);
	int nstdfilteroutlier(const DAPyDataFrame& df, double n = 3, int axis = 0, const QList< int > index = QList< int >());
	// 替换规定界限外的异常值
	bool clipoutlier();
	bool clipoutlier(const DAPyDataFrame& df, double lower = 0.0, double upper = 0.0, int axis = 0);
Q_SIGNALS:
	/**
	 * @brief 选中的列或者类型发生了变化
	 * @param column 列 如果返回空说明影响的列不确定,如果多选，返回
	 * @param dt 类型
	 * @note 此函数主要是通知主界面ribbon上面的类型变化，调用setDataframeOperateCurrentDType
	 */
	void selectTypeChanged(const QList< int >& column, DA::DAPyDType dt);
private Q_SLOTS:
	// 表格点击
	void onTableViewClicked(const QModelIndex& index);

protected:
	void changeEvent(QEvent* e);

private:
	Ui::DADataOperateOfDataFrameWidget* ui;
	DAData mData;
    DAPyDataFrameTableModule* mModel { nullptr };

    DADialogDataframeColumnCastToNumeric* mDialogCastNumArgs { nullptr };
    DADialogDataframeColumnCastToDatetime* mDialogCastDatetimeArgs { nullptr };
    DADialogDataFrameFillna* mDialogDataFrameFillna { nullptr };
    DADialogDataFrameFillInterpolate* mDialogDataFrameFillInterpolate { nullptr };
    DADialogDataFrameClipOutlier* mDialogDataFrameClipOutlier { nullptr };
};
}  // end of namespace DA
#endif  // DADATAOPERATEOFDATAFRAMEWIDGET_H
