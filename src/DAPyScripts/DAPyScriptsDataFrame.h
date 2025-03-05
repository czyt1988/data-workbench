﻿#ifndef DAPYSCRIPTSDATAFRAME_H
#define DAPYSCRIPTSDATAFRAME_H
#include "DAPyScriptsGlobal.h"
#include "DAPyModule.h"
#include <QString>
#include <QList>
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
	~DAPyScriptsDataFrame();
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
	// 保存为pickle
	bool to_pickle(const DAPyDataFrame& df, const QString& path) noexcept;
	// 从pickle加载
	bool from_pickle(DAPyDataFrame& df, const QString& path) noexcept;
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
				int axis                   = 0,
				const QString& how         = QStringLiteral("any"),
				const QList< int >& indexs = QList< int >(),
				int thresh                 = -1);
	//	dropduplicates(keep = "first")
	bool dropduplicates(DAPyDataFrame& df, const QString& keep, const QList< int >& indexs);
	// fillna()
	bool fillna(DAPyDataFrame& df, double value = 0.0, int limit = -1);
};
}  // namespace DA
#endif  // DAPYSCRIPTSDATAFRAME_H
