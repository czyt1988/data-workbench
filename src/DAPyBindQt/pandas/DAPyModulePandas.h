#ifndef DAPYPANDAS_H
#define DAPYPANDAS_H
#include "DAPyBindQtGlobal.h"
#include "DAPyDataFrame.h"
#include <QVariant>
#include "DAPyModule.h"
namespace DA
{
/**
 * @brief 对应pandas的包
 */
class DAPYBINDQT_API DAPyModulePandas : public DAPyModule
{
	DA_DECLARE_PRIVATE(DAPyModulePandas)

public:
	DAPyModulePandas();
	~DAPyModulePandas();
	// 获取实例
	static DAPyModulePandas& getInstance();
	// 析构
	void finalize();
	// 获取最后的错误
	QString getLastErrorString();

public:
	// 导入模块
	bool import();

public:
	// 判断是否为pandas.series
	bool isInstanceSeries(const pybind11::object& obj) const;
	bool isInstanceDataFrame(const pybind11::object& obj) const;
	bool isInstanceIndex(const pybind11::object& obj) const;
	static bool isInstanceDataFrame_(const pybind11::object& obj);
	// pandas.read_csv
	DAPyDataFrame read_csv(const QString& path, const QVariantHash& args = QVariantHash());
};
}  // namespace DA
#endif  // DAPANDAS_H
