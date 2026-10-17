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
	// ===== 计算列（004 T1.23）=====
	// 计算列求值结果（对应 da_dataframe.da_eval_column）
	struct EvalColumnResult
	{
		bool ok { false };         ///< 求值是否成功（语法错误/引用不存在列等抛错为 false，df 不变）
		bool hadColumn { false };  ///< 求值前目标列是否已存在（false=新建列，undo 应删除该列）
		DAPySeries oldColumn;      ///< hadColumn=true 时为求值前旧列快照（列级撤销记录，D4）
		QString errorMessage;      ///< 失败时的异常信息（英文诊断，对话框提示用）
	};
	// 计算列求值：把 body（RHS 表达式，列引用可含反引号）求值结果写入 targetColumn 列，
	// 返回旧列快照（对应 da_dataframe.da_eval_column；目标列名可含空格，健壮路径见算子注释）
	EvalColumnResult evalColumn(DAPyDataFrame& df, const QString& targetColumn, const QString& body) noexcept;
	// 提取表达式体引用的列名（与 df 实际列的交集，按列序返回，对应 da_dataframe.da_expr_depends）；
	// 表达式无法解析（语法错误）返回空列表且 *ok=false（errorMessage 输出异常诊断）
	QStringList exprDepends(const DAPyDataFrame& df, const QString& body, bool* ok = nullptr, QString* errorMessage = nullptr) noexcept;
	// 列名在 eval 表达式里是否需要反引号包裹（对应 da_dataframe.da_needs_backtick，R20）
	bool needsBacktick(const QString& name) noexcept;
	// 取头部前 n 行的独立副本（对话框预览与执行同一算子，对应 da_dataframe.da_head_copy）
	DAPyDataFrame headCopy(const DAPyDataFrame& df, int n) noexcept;
	// 取某列的独立副本（分列/合列命令列级快照，对应 da_dataframe.da_get_column_copy）；越界返回 none
	DAPySeries getColumnCopy(const DAPyDataFrame& df, int col) noexcept;
	// 按列名把 series 写回列（计算列重算命令 undo 还原，对应 da_dataframe.da_set_column）
	bool setColumn(DAPyDataFrame& df, const QString& name, const DAPySeries& series) noexcept;
	// ===== 分列 / 合并列（004 T1.9）=====
	// 分列（对应 da_dataframe.da_split_column，就地：结果列插入原列右侧，原列保留）：
	// mode = "char"/"regex"/"width"；maxColumns<=0 表示不限；overflow = "discard"/"keep"/"error"；
	// newNames 空 = 自动命名 "{源列名}_{i}"。成功返回实际插入的结果列名（按顺序）；
	// 失败（参数/正则/溢出策略抛错）返回空列表且 errorMessage 输出诊断
	QStringList splitColumn(DAPyDataFrame& df,
	                        int col,
	                        const QString& mode,
	                        const QString& delimiter,
	                        const QString& pattern,
	                        const QList< int >& widths,
	                        int maxColumns,
	                        const QString& overflow,
	                        bool inferNumeric,
	                        const QStringList& newNames,
	                        QString* errorMessage = nullptr) noexcept;
	// 合并列（对应 da_dataframe.da_merge_columns，就地：结果列插到最左源列位置并删除源列）；
	// 成功返回插入位置（>=0），失败返回 -1 且 errorMessage 输出诊断
	int mergeColumns(DAPyDataFrame& df, const QList< int >& cols, const QString& sep, bool skipNa, const QString& name, QString* errorMessage = nullptr) noexcept;
	// ===== 分组聚合（004 T1.22）=====
	// 分组聚合（对应 da_dataframe.da_groupby_aggregate）：产出长格式聚合结果表
	//（新 DataFrame，不改原表）；聚合函数集合与透视表对齐（mean/sum/size）。
	// values/funcs 一一对应；失败返回 none 的 DAPyDataFrame 且 errorMessage 输出诊断
	DAPyDataFrame groupbyAggregate(const DAPyDataFrame& df,
	                               const QStringList& keys,
	                               const QStringList& values,
	                               const QStringList& funcs,
	                               QString* errorMessage = nullptr) noexcept;
	// 分组数（对话框「预估结果行数」，对应 da_dataframe.da_group_count）；失败返回 -1
	qint64 groupCount(const DAPyDataFrame& df, const QStringList& keys) noexcept;
	// ===== 形状变换（004 T1.27）=====
	// 转置（对应 da_dataframe.da_transpose）：产出新 DataFrame（不改原表，R14）；
	// 失败返回显式 none 的 DAPyDataFrame
	DAPyDataFrame transpose(const DAPyDataFrame& df, QString* errorMessage = nullptr) noexcept;
	// 宽转长 melt（对应 da_dataframe.da_melt）：产出新 DataFrame（不改原表）；
	// valueVars 空 = 除 idVars 外全部列（pandas 语义）；失败返回显式 none
	DAPyDataFrame melt(const DAPyDataFrame& df,
	                   const QStringList& idVars,
	                   const QStringList& valueVars,
	                   const QString& varName,
	                   const QString& valueName,
	                   QString* errorMessage = nullptr) noexcept;
	// 多键稳定排序，对应da_dataframe.da_sort：keys为(列位置,是否升序)列表（顺序即优先级），
	// 成功返回true；perm非空时输出置换（perm[i]=新表第i行在旧表中的行号，与DATableKeyRemap语义一致）
	bool sort(DAPyDataFrame& df, const QVector< QPair< int, bool > >& keys, QVector< int >* perm = nullptr) noexcept;
	// 按给定置换就地重排行（new[i]=old[perm[i]]），对应da_dataframe.da_reorder_rows，
	// 排序命令的redo/undo用它施加正/逆置换
	bool reorderRows(DAPyDataFrame& df, const QVector< int >& perm) noexcept;
	// 列统计结果（条件格式色阶/数据条端点，对应 da_dataframe.da_column_stats）
	struct ColumnStatsResult
	{
		bool ok = false;  ///< 计算是否成功（列越界/全NaN/非数值列为 false）
		double minValue { 0.0 };
		double maxValue { 0.0 };
		double medianValue { 0.0 };
	};
	// 单列 min/max/median 全表 O(n) 归约（NaN 排除，不采样不近似）；
	// 列越界抛错或全 NaN 返回 ok=false
	ColumnStatsResult columnStats(const DAPyDataFrame& df, int col) noexcept;
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
