#ifndef DACOMMANDSDATAFRAME_H
#define DACOMMANDSDATAFRAME_H
#include <QUndoCommand>
#include <QPoint>
#include <optional>
#include "DAGuiAPI.h"
#include "DACommandWithRedoCount.h"
#include "DACommandWithTemporaryData.h"
#include "DAData.h"
#include "numpy/DAPyDType.h"
#include "pandas/DAPySeries.h"
class QHeaderView;

namespace DA
{
class DAPyDataFrameTableModel;
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
						   DAPyDataFrameTableModel* model = nullptr,
						   QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	DAPyDataFrame mDataframe;
	int mRow;
	int mCol;
	QVariant mOldData;
	QVariant mNewData;
	DAPyDataFrameTableModel* mModel;
};

/**
 * @brief 插入一个空行
 */
class DAGUI_API DACommandDataFrame_insertNanRow : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_insertNanRow(const DAPyDataFrame& df,
									int row,
									DAPyDataFrameTableModel* model = nullptr,
									QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	int mRow;
	DAPyDataFrameTableModel* mModel;
};

/**
 * @brief 插入列
 */
class DAGUI_API DACommandDataFrame_insertColumn : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_insertColumn(const DAPyDataFrame& df,
									int col,
									const QString& name,
									const QVariant& defaultvalue,
									DAPyDataFrameTableModel* model = nullptr,
									QUndoCommand* par              = nullptr);
	DACommandDataFrame_insertColumn(const DAPyDataFrame& df,
									int col,
									const QString& name,
									const QVariant& start,
									const QVariant& stop,
									DAPyDataFrameTableModel* model = nullptr,
									QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	bool mIsRangeMode;  ///< 标记是否是一个范围生成
	int mCol;           ///< 插入的列
	QString mName;
	QVariant mDefaultvalue;
	QVariant mStart;
	QVariant mStop;
	DAPyDataFrameTableModel* mModel;
	DAPySeries mInsertedSeries;
};

/**
 * @brief 删除行
 */
class DAGUI_API DACommandDataFrame_dropIRow : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_dropIRow(const DAPyDataFrame& df,
								const QList< int >& index,
								DAPyDataFrameTableModel* model = nullptr,
								QUndoCommand* par              = nullptr);

	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	DAPyDataFrameTableModel* mModel;
};

/**
 * @brief 删除列
 */
class DAGUI_API DACommandDataFrame_dropIColumn : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_dropIColumn(const DAPyDataFrame& df,
								   const QList< int >& index,
								   DAPyDataFrameTableModel* model = nullptr,
								   QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	DAPyDataFrameTableModel* mModel;
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
class DAGUI_API DACommandDataFrame_astype : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_astype(const DAPyDataFrame& df,
							  const QList< int >& index,
							  const DAPyDType& dt,
							  DAPyDataFrameTableModel* model = nullptr,
							  QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	DAPyDType mDtype;
	DAPyDataFrameTableModel* mModel;
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
							  DAPyDataFrameTableModel* model = nullptr,
							  QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	DAPyDataFrame mDataframe;
	QList< int > mRows;
	QList< int > mColumns;
	DAPyDataFrameTableModel* mModel;
	QList< pybind11::object > mOlddatas;
};

/**
 * @brief dropnan
 */
class DAGUI_API DACommandDataFrame_dropna : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_dropna(const DAPyDataFrame& df,
							  DAPyDataFrameTableModel* model = nullptr,
							  int axis                       = 0,
							  const QString& how             = QStringLiteral("any"),
							  const QList< int >& index      = QList< int >(),
							  std::optional< int > thresh    = std::nullopt,
							  QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

	int getDropedCount() const;

private:
    DAPyDataFrameTableModel* mModel { nullptr };
    int mAxis { 0 };
	QString mHow;
	QList< int > mIndex;
	std::optional< int > mThresh;
    int mDropedCount { 0 };  ///< 记录删除了多少行或列
};

/**
 * @brief fillnan命令
 */
class DAGUI_API DACommandDataFrame_fillna : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_fillna(const DAPyDataFrame& df,
							  DAPyDataFrameTableModel* model = nullptr,
							  double value                   = 0.0,
							  int limit                      = -1,
							  QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
    DAPyDataFrameTableModel* mModel { nullptr };
    double mValue { 0.0 };
    int mLimit { -1 };
};

/**
 * @brief interpolate命令
 */
class DAGUI_API DACommandDataFrame_interpolate : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_interpolate(const DAPyDataFrame& df,
								   DAPyDataFrameTableModel* model = nullptr,
								   const QString& method          = QStringLiteral("spline"),
								   int order                      = 1,
								   int limit                      = -1,
								   QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
    DAPyDataFrameTableModel* mModel { nullptr };
	QString mMethod;
    int mOrder { 1 };
    int mLimit { -1 };
};

/**
 * @brief ffillnan命令
 */
class DAGUI_API DACommandDataFrame_ffillna : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_ffillna(const DAPyDataFrame& df,
							   DAPyDataFrameTableModel* model = nullptr,
							   int axis                       = 0,
							   int limit                      = -1,
							   QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
    DAPyDataFrameTableModel* mModel { nullptr };
    int mAxis { 0 };
    int mLimit { -1 };
};

/**
 * @brief bfillnan命令
 */
class DAGUI_API DACommandDataFrame_bfillna : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_bfillna(const DAPyDataFrame& df,
							   DAPyDataFrameTableModel* model = nullptr,
							   int axis                       = 0,
							   int limit                      = -1,
							   QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
    DAPyDataFrameTableModel* mModel { nullptr };
    int mAxis { 0 };
    int mLimit { -1 };
};

/**
 * @brief dropduplicates
 */
class DAGUI_API DACommandDataFrame_dropduplicates : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_dropduplicates(const DAPyDataFrame& df,
									  DAPyDataFrameTableModel* model = nullptr,
									  const QString& keep            = QStringLiteral("first"),
									  const QList< int >& index      = QList< int >(),
									  QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

	int getDropedCount() const;

private:
    DAPyDataFrameTableModel* mModel { nullptr };
	QString mKeep;
	QList< int > mIndex;
    int mDropedCount { 0 };  ///< 记录删除了多少行或列
};

/**
 * @brief nstdfilteroutlier
 */
class DAGUI_API DACommandDataFrame_nstdfilteroutlier : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_nstdfilteroutlier(const DAPyDataFrame& df,
										 DAPyDataFrameTableModel* model = nullptr,
										 double n                       = 3,
										 int axis                       = 1,
										 const QList< int >& index      = QList< int >(),
										 QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;
	int getDropedCount() const;

private:
    DAPyDataFrameTableModel* mModel { nullptr };
    double mN { 3.0 };
    int mAxis { 0 };
	QList< int > mIndex;
    int mDropedCount { 0 };  ///< 记录删除了多少行或列
};

/**
 * @brief clipoutlier
 */
class DAGUI_API DACommandDataFrame_clipoutlier : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_clipoutlier(const DAPyDataFrame& df,
								   DAPyDataFrameTableModel* model = nullptr,
								   double lowervalue              = 0.0,
								   double uppervalue              = 0.0,
								   int axis                       = 0,
								   QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
    DAPyDataFrameTableModel* mModel { nullptr };
    double mlowervalue { 0.0 };
    double mUppervalue { 0.0 };
    int mAxis { 0 };
	QList< int > mIndex;
};

/**
 * @brief evaldatas
 */
class DAGUI_API DACommandDataFrame_evalDatas : public DACommandWithTemporaryData
{
public:
    DACommandDataFrame_evalDatas(const DAPyDataFrame& df,
                                 const QString& exper,
                                 DAPyDataFrameTableModel* model = nullptr,
                                 QUndoCommand* par              = nullptr);
    virtual void undo() override;
    virtual bool exec() override;

private:
    QString mExper;
    DAPyDataFrameTableModel* mModel { nullptr };
};

/**
 * @brief querydatas
 */
class DAGUI_API DACommandDataFrame_querydatas : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_querydatas(const DAPyDataFrame& df,
								  const QString& exper,
								  DAPyDataFrameTableModel* model = nullptr,
								  QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QString mExper;
    DAPyDataFrameTableModel* mModel { nullptr };
};

/**
 * @brief searchdata
 */
class DAGUI_API DACommandDataFrame_searchdata : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_searchdata(const DAPyDataFrame& df,
								  const QString& exper,
								  DAPyDataFrameTableModel* model = nullptr,
								  QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QString mExper;
	DAPyDataFrameTableModel* mModel{ nullptr };
};

/**
 * @brief dataselect
 */

class DAGUI_API DACommandDataFrame_filterByColumn : public DACommandWithTemporaryData
{
public:
    DACommandDataFrame_filterByColumn(const DAPyDataFrame& df,
                                      double lowervalue,
                                      double uppervalue,
                                      const QString& index,
                                      DAPyDataFrameTableModel* model = nullptr,
                                      QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
    DAPyDataFrameTableModel* mModel { nullptr };
    double mlowervalue { 0.0 };
    double mUppervalue { 0.0 };
	QString mIndex;
};

/**
 * @brief sort
 */
class DAGUI_API DACommandDataFrame_sort : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_sort(const DAPyDataFrame& df,
							const QString& by,
							const bool ascending,
							DAPyDataFrameTableModel* model = nullptr,
							QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QString mBy;
	bool mAscending;
    DAPyDataFrameTableModel* mModel { nullptr };
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_castNum : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_castNum(const DAPyDataFrame& df,
							   const QList< int >& index,
							   const pybind11::dict& args,
							   DAPyDataFrameTableModel* model = nullptr,
							   QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	pybind11::dict mArgs;
	DAPyDataFrameTableModel* mModel;
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_castDatetime : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_castDatetime(const DAPyDataFrame& df,
									const QList< int >& index,
									const pybind11::dict& args,
									DAPyDataFrameTableModel* model = nullptr,
									QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	pybind11::dict mArgs;
	DAPyDataFrameTableModel* mModel;
};

/**
 * @brief 转换列为索引
 */
class DAGUI_API DACommandDataFrame_setIndex : public DACommandWithTemporaryData
{
public:
	DACommandDataFrame_setIndex(const DAPyDataFrame& df,
								const QList< int >& index,
								QHeaderView* hv                = nullptr,
								DAPyDataFrameTableModel* model = nullptr,
								QUndoCommand* par              = nullptr);
	virtual void undo() override;
	virtual bool exec() override;

private:
	QList< int > mIndex;
	DAPyDataFrameTableModel* mModel;
};
}  // end of namespace DA
#endif  // DACOMMANDSDATAFRAME_H
