#ifndef DACOMMANDSDATAFRAME_H
#define DACOMMANDSDATAFRAME_H
#include <QUndoCommand>
#include <QPoint>
#include "DAGuiAPI.h"
#include "DACommandWithRedoCount.h"
#include "DACommandWithTemplateData.h"
#include "DAData.h"
#include "numpy/DAPyDType.h"
#include "pandas/DAPySeries.h"
class QHeaderView;

namespace DA
{
class DAPyDataFrameTableModule;
/**
 * @file DataManager相关的命令
 *
 */

/**
 * @brief 添加变量命令
 *
 * redo会调用iat设置值，但是，如果值没有设置成功，将会把m_isSuccess设置为false，这时调用undo不做任何处理
 *
 * 因此在对一些场合，需要判断是否设置成功的地方，可以按照如下方式进行操作
 * @code
 * bool DAPyDataFrameTableModule::setData(const QModelIndex& index, const QVariant& value, int role)
 * {
 *     if (Qt::EditRole != role) {
 *         return false;
 *     }
 *     if (!index.isValid() || d_ptr->_dataframe.isNone()) {
 *         return false;
 *     }
 *     std::pair< std::size_t, std::size_t > shape = d_ptr->_dataframe.shape();
 *     if (index.row() >= (int)shape.first || index.column() >= (int)shape.second) {
 *         return false;
 *     }
 *     QVariant olddata                = d_ptr->_dataframe.iat(index.row(), index.column());
 *     DACommandDataFrame_iat* cmd_iat = new DACommandDataFrame_iat(d_ptr->_dataframe, index.row(), index.column(),
 * olddata, value); d_ptr->_undoStack->push(cmd_iat); if (!cmd_iat->isSetSuccess()) {
 *         //没设置成功，undo回退一步
 *         d_ptr->_undoStack->undo();
 *         return false;
 *     }
 *     //这里说明设置成功了
 *     return true;
 * }
 * @endcode
 */
class DAGUI_API DACommandDataFrame_iat : public DACommandWithRedoCount
{
public:
	DACommandDataFrame_iat(const DAPyDataFrame& df,
						   int row,
						   int col,
						   const QVariant& olddata,
						   const QVariant& newdata,
						   DAPyDataFrameTableModule* model = nullptr,
						   QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	DAPyDataFrame mDataframe;
	int mRow;
	int mCol;
	QVariant mOldData;
	QVariant mNewData;
	DAPyDataFrameTableModule* mModel;
};

/**
 * @brief 插入一个空行
 */
class DAGUI_API DACommandDataFrame_insertNanRow : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_insertNanRow(const DAPyDataFrame& df,
									int row,
									DAPyDataFrameTableModule* model = nullptr,
									QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	int mRow;
	DAPyDataFrameTableModule* mModel;
};

/**
 * @brief 插入列
 */
class DAGUI_API DACommandDataFrame_insertColumn : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_insertColumn(const DAPyDataFrame& df,
									int col,
									const QString& name,
									const QVariant& defaultvalue,
									DAPyDataFrameTableModule* model = nullptr,
									QUndoCommand* par               = nullptr);
	DACommandDataFrame_insertColumn(const DAPyDataFrame& df,
									int col,
									const QString& name,
									const QVariant& start,
									const QVariant& stop,
									DAPyDataFrameTableModule* model = nullptr,
									QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	bool mIsRangeMode;  ///< 标记是否是一个范围生成
	int mCol;           ///< 插入的列
	QString mName;
	QVariant mDefaultvalue;
	QVariant mStart;
	QVariant mStop;
	DAPyDataFrameTableModule* mModel;
	DAPySeries mInsertedSeries;
};

/**
 * @brief 删除行
 */
class DAGUI_API DACommandDataFrame_dropIRow : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_dropIRow(const DAPyDataFrame& df,
								const QList< int >& index,
								DAPyDataFrameTableModule* model = nullptr,
								QUndoCommand* par               = nullptr);

	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	DAPyDataFrameTableModule* mModel;
};

/**
 * @brief 删除列
 */
class DAGUI_API DACommandDataFrame_dropIColumn : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_dropIColumn(const DAPyDataFrame& df,
								   const QList< int >& index,
								   DAPyDataFrameTableModule* model = nullptr,
								   QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	DAPyDataFrameTableModule* mModel;
};

/**
 * @brief 更改列名
 */
class DAGUI_API DACommandDataFrame_renameColumns : public DACommandWithRedoCount
{
public:
	DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
									 const QList< QString >& cols,
									 QHeaderView* hv   = nullptr,
									 QUndoCommand* par = nullptr);
	DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
									 const QList< QString >& cols,
									 const QList< QString >& oldcols,
									 QHeaderView* hv   = nullptr,
									 QUndoCommand* par = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	DAPyDataFrame mDataframe;
	QList< QString > mCols;
	QHeaderView* mHeaderView;
	QList< QString > mOldcols;
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_astype : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_astype(const DAPyDataFrame& df,
							  const QList< int >& index,
							  const DAPyDType& dt,
							  DAPyDataFrameTableModule* model = nullptr,
							  QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	DAPyDType mDtype;
	DAPyDataFrameTableModule* mModel;
};

/**
 * @brief 设置为nan
 */
class DAGUI_API DACommandDataFrame_setnan : public DACommandWithRedoCount
{
public:
	DACommandDataFrame_setnan(const DAPyDataFrame& df,
							  const QList< int >& rows,
							  const QList< int >& columns,
							  DAPyDataFrameTableModule* model = nullptr,
							  QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	DAPyDataFrame mDataframe;
	QList< int > mRows;
	QList< int > mColumns;
	DAPyDataFrameTableModule* mModel;
	QList< pybind11::object > mOlddatas;
};

/**
 * @brief dropnan
 */
class DAGUI_API DACommandDataFrame_dropna : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_dropna(const DAPyDataFrame& df,
							  DAPyDataFrameTableModule* model = nullptr,
							  int axis                        = 0,
							  const QString& how              = QStringLiteral("any"),
							  const QList< int >& index       = QList< int >(),
							  int thresh                      = -1,
							  QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

	int getDropedCount() const;

private:
	DAPyDataFrameTableModule* mModel { nullptr };
	int mAxis { 0 };
	QString mHow;
	QList< int > mIndex;
	int mThresh { -1 };
	int mDropedCount { 0 };  ///< 记录删除了多少行或列
};

/**
 * @brief dropduplicates
 */
class DAGUI_API DACommandDataFrame_dropduplicates : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_dropduplicates(const DAPyDataFrame& df,
									  DAPyDataFrameTableModule* model = nullptr,
									  const QString& keep             = QStringLiteral("first"),
									  const QList< int >& index       = QList< int >(),
									  QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

	int getDropedCount() const;

private:
	DAPyDataFrameTableModule* mModel { nullptr };
	QString mKeep;
	QList< int > mIndex;
	int mDropedCount { 0 };  ///< 记录删除了多少行或列
};

/**
 * @brief fillnan命令
 */
class DAGUI_API DACommandDataFrame_fillna : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_fillna(const DAPyDataFrame& df,
							  DAPyDataFrameTableModule* model = nullptr,
							  double value                    = 0.0,
							  int limit                       = -1,
							  QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	DAPyDataFrameTableModule* mModel { nullptr };
	double mValue { 0.0 };
	int mLimit { -1 };
};

/**
 * @brief interpolate命令
 */
class DAGUI_API DACommandDataFrame_interpolate : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_interpolate(const DAPyDataFrame& df,
								   DAPyDataFrameTableModule* model = nullptr,
								   const QString& method           = QStringLiteral("spline"),
								   int order                       = 1,
								   int limit                       = -1,
								   QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	DAPyDataFrameTableModule* mModel { nullptr };
	QString mMethod;
	int mOrder { 1 };
	int mLimit { -1 };
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_castNum : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_castNum(const DAPyDataFrame& df,
							   const QList< int >& index,
							   const pybind11::dict& args,
							   DAPyDataFrameTableModule* model = nullptr,
							   QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	pybind11::dict mArgs;
	DAPyDataFrameTableModule* mModel;
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_castDatetime : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_castDatetime(const DAPyDataFrame& df,
									const QList< int >& index,
									const pybind11::dict& args,
									DAPyDataFrameTableModule* model = nullptr,
									QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	pybind11::dict mArgs;
	DAPyDataFrameTableModule* mModel;
};

/**
 * @brief 转换列为索引
 */
class DAGUI_API DACommandDataFrame_setIndex : public DACommandWithTemplateData
{
public:
	DACommandDataFrame_setIndex(const DAPyDataFrame& df,
								const QList< int >& index,
								QHeaderView* hv                 = nullptr,
								DAPyDataFrameTableModule* model = nullptr,
								QUndoCommand* par               = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	DAPyDataFrameTableModule* mModel;
};
}  // end of namespace DA
#endif  // DACOMMANDSDATAFRAME_H
