#ifndef DAPYSCRIPTSDATAFRAME_H
#define DAPYSCRIPTSDATAFRAME_H
#include "DAPyScriptsGlobal.h"
#include "DAPyModule.h"
#include <optional>
#include <QString>
#include <QList>
#include <QPair>
#include <QVector>
#include "DAPyObjectWrapper.h"
#include "numpy/DAPyDType.h"
#include "pandas/DAPyDataFrame.h"

namespace DA
{
/**
 * @brief 对da_dataframe.py的封装，集成了dataframe的操作
 *
 * 需要在bool DAPyScripts::initScripts(QString* err)中import进来
 */
class DAPYSCRIPTS_API DAPyScriptsDataFrame : public DAPyModule
{
public:
	DAPyScriptsDataFrame(bool autoImport = true);
	DAPyScriptsDataFrame(const pybind11::object& obj);
	~DAPyScriptsDataFrame() override;
	// 引入
	bool import() noexcept;

public:
	// 移除dataframe的行，对应da_dataframe.da_drop_irow
	bool drop_irow(DAPyDataFrame& df, const QList< int >& index) noexcept;
	// 移除dataframe的列，对应da_dataframe.da_drop_icolumn
	bool drop_icolumn(DAPyDataFrame& df, const QList< int >& index) noexcept;
	// 插入一个空行
	bool insert_nanrow(DAPyDataFrame& df, int r) noexcept;
	// 插入列-全为nan
	bool insert_column(DAPyDataFrame& df, int c, const QString& name, const QVariant& defaultvalue = QVariant()) noexcept;
	bool insert_column(DAPyDataFrame& df, int c, const QString& name, const QVariant& start, const QVariant& stop) noexcept;
	//
	// 保存为txt/csv
	bool to_csv(const DAPyDataFrame& df, const QString& path, const QString& sep) noexcept;
	// 保存为xlsx
	bool to_excel(const DAPyDataFrame& df, const QString& path) noexcept;
	// 保存为pickle
	bool to_pickle(const DAPyDataFrame& df, const QString& path) noexcept;
	// 保存为parquet
	bool to_parquet(const DAPyDataFrame& df, const QString& path) noexcept;
	// 从pickle加载
	bool from_pickle(DAPyDataFrame& df, const QString& path) noexcept;
	// 从parquet加载
	bool from_parquet(DAPyDataFrame& df, const QString& path) noexcept;
	// 类型转换
	bool astype(DAPyDataFrame& df, const QList< int >& colsIndex, const DAPyDType& dt) noexcept;
	// 设置nan值
	bool setnan(DAPyDataFrame& df, const QList< int >& rowsIndex, const QList< int >& colsIndex) noexcept;
	// 转换为数值 :da_cast_to_num
	bool cast_to_num(DAPyDataFrame& df, const QList< int >& colsIndex, pybind11::dict args) noexcept;
	bool cast_to_datetime(DAPyDataFrame& df, const QList< int >& colsIndex, pybind11::dict args) noexcept;
	// 设置某列为index
	bool set_index(DAPyDataFrame& df, const QList< int >& colsIndex) noexcept;
	// 提取一列，此函数如果失败返回一个none
	DAPySeries itake_column(DAPyDataFrame& df, int col) noexcept;
	// 在col位置插入series
	bool insert_at(DAPyDataFrame& df, int col, const DAPySeries& series) noexcept;
	// dropna(axis=0,how="any")
	bool dropna(DAPyDataFrame& df,
				int axis                    = 0,
				const QString& how          = QStringLiteral("any"),
				const QList< int >& indexs  = QList< int >(),
				std::optional< int > thresh = std::nullopt) noexcept;
	// fillna()
	bool fillna(DAPyDataFrame& df, double value, int limit) noexcept;
	// interpolate()
	bool interpolate(DAPyDataFrame& df, const QString& method, int order, int limit) noexcept;
	// ffillna()
	bool ffillna(DAPyDataFrame& df, int axis, int limit) noexcept;
	// bfillna()
	bool bfillna(DAPyDataFrame& df, int axis, int limit) noexcept;
	//	dropduplicates(keep = "first","last")
	bool dropduplicates(DAPyDataFrame& df, const QString& keep, const QList< int >& indexs) noexcept;
	// nstdfilteroutlier()
	bool nstdfilteroutlier(DAPyDataFrame& df, double n, int axis, const QList< int >& indexs) noexcept;
	// clipoutlier()
	bool clipoutlier(DAPyDataFrame& df, double lowervalue, double uppervalue, int axis) noexcept;
	// querydatas()
	bool queryDatas(DAPyDataFrame& df, const QString& expr) noexcept;
	// searchdata()（旧数值等值入口，仅为插件旧检索对话框保留，插件收口后移除）
	QList< QPair< int, int > > searchData(const DAPyDataFrame& df, const QString& value) noexcept;
	// 查找结果（紧凑命中数组 + 真实总数，§4.2 硬约束③：超上限截断时 totalHits > rows.size()）
	struct SearchDataResult
	{
		QVector< int > rows;        ///< 命中行（数据源绝对行号，行主序）
		QVector< int > cols;        ///< 命中列
		int totalHits        = 0;   ///< 真实命中总数（含被截断部分）
		bool ok              = false;  ///< 调用是否成功（无效正则等抛错为 false）
		bool truncated() const
		{
			return totalHits > rows.size();
		}
	};
	// 查找（扩展选项，对应 da_search_data）：大小写/正则/整格 + 行/列范围闭区间
	//（nullopt=不限），命中超 maxHits 截断（totalHits 返回真实总数）
	SearchDataResult searchData(const DAPyDataFrame& df,
	                            const QString& value,
	                            bool caseSensitive,
	                            bool useRegex,
	                            bool wholeCell,
	                            const std::optional< QPair< int, int > >& rowRange = std::nullopt,
	                            const std::optional< QPair< int, int > >& colRange = std::nullopt,
	                            int maxHits                                       = 100000) noexcept;
	// 查找替换（对应 da_find_and_replace）：按同一匹配语义就地替换，
	// 变更清单输出到 rows/cols/oldValues/newValues（Python list，值记录撤销用 D4）；
	// 出参可空（超阈值降级路径只要替换结果不要清单）。正则无效等抛错返回 false
	bool findReplace(DAPyDataFrame& df,
	                 const QString& find,
	                 const QString& replace,
	                 bool caseSensitive,
	                 bool useRegex,
	                 bool wholeCell,
	                 const std::optional< QPair< int, int > >& rowRange = std::nullopt,
	                 const std::optional< QPair< int, int > >& colRange = std::nullopt,
	                 QVector< int >* rows                               = nullptr,
	                 QVector< int >* cols                               = nullptr,
	                 pybind11::list* oldValues                          = nullptr,
	                 pybind11::list* newValues                          = nullptr) noexcept;
	// 按变更清单批量写回单元格值（对应 da_write_cells，替换命令 undo/redo 用）
	bool writeCells(DAPyDataFrame& df, const QVector< int >& rows, const QVector< int >& cols, const pybind11::list& values) noexcept;
	// eval
	bool evalDatas(DAPyDataFrame& df, const QString& expr) noexcept;
	// 排序sort()（旧单键入口，仅为插件旧排序命令保留，插件收口后移除）
	bool sort(DAPyDataFrame& df, const QString& by, bool ascending) noexcept;
	// 多键稳定排序，对应da_dataframe.da_sort：keys为(列位置,是否升序)列表（顺序即优先级），
	// 成功返回true；perm非空时输出置换（perm[i]=新表第i行在旧表中的行号，与DATableKeyRemap语义一致）
	bool sort(DAPyDataFrame& df, const QVector< QPair< int, bool > >& keys, QVector< int >* perm = nullptr) noexcept;
	// 按给定置换就地重排行（new[i]=old[perm[i]]），对应da_dataframe.da_reorder_rows，
	// 排序命令的redo/undo用它施加正/逆置换
	bool reorderRows(DAPyDataFrame& df, const QVector< int >& perm) noexcept;
	// dataselect()
	bool dataselect(DAPyDataFrame& df, double lowervalue, double uppervalue, const QString& index) noexcept;

	// 创建数据透视表
	DAPyDataFrame pivotTable(const DAPyDataFrame& df,
							 const QStringList& value,
							 const QStringList& index,
							 const QStringList& columns,
							 const QString& aggfunc,
							 bool margins,
							 const QString& marginsName,
							 bool sort) noexcept;
};
}  // namespace DA
#endif  // DAPYSCRIPTSDATAFRAME_H
