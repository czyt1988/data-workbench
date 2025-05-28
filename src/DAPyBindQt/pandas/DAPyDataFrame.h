#ifndef DAPYDATAFRAME_H
#define DAPYDATAFRAME_H
#include "DAPyBindQtGlobal.h"
#include "DAPyObjectWrapper.h"
#include "DAPyIndex.h"
#include "DAPybind11InQt.h"
#include "DAPySeries.h"
#include <QVariant>
#include <QDebug>
#include <QList>
namespace DA
{
/**
 * @brief 非模板类的DataFrame
 *
 * 此类相当是对python Dataframe的封装
 */
class DAPYBINDQT_API DAPyDataFrame : public DAPyObjectWrapper
{
public:
	DAPyDataFrame();
	DAPyDataFrame(const DAPyDataFrame& df);
	DAPyDataFrame(const DAPyObjectWrapper& df);
	DAPyDataFrame(DAPyDataFrame&& df);
	DAPyDataFrame(const pybind11::object& obj);
	DAPyDataFrame(pybind11::object&& obj);
	~DAPyDataFrame();
	// 获取列
	// 注意pandas.dataframe的列名是可以任意类型，如果列名是数字，那么熟通过数字来获取而不是用字符串，
	// 这里重写的operator[](int n)是针对iloc，并不是基于数字列名的获取
	DAPySeries operator[](const QString& n) const;
	DAPySeries operator[](int n) const;
	DAPyDataFrame& operator=(const pybind11::object& obj);
	DAPyDataFrame& operator=(const DAPyDataFrame& obj);
	DAPyDataFrame& operator=(const DAPyObjectWrapper& obj);
	DAPyDataFrame& operator=(pybind11::object&& obj);
	DAPyDataFrame& operator=(DAPyDataFrame&& obj);
	DAPyDataFrame& operator=(DAPyObjectWrapper&& obj);

public:
	// DataFrame.columns
	QList< QString > columns() const;
    QVariant columnName(std::size_t i) const;
	bool columns(std::size_t i, const QString& name);
	bool columns(const QList< QString >& cols);
	// DataFrame.empty
	bool empty() const;
	// DataFrame.shape
	std::pair< std::size_t, std::size_t > shape() const;
	// DataFrame.size
	std::size_t size() const;
	// DataFrame.iat
	QVariant iat(std::size_t r, std::size_t c) const;
	pybind11::object iatObj(std::size_t r, std::size_t c) const;
	bool iat(std::size_t r, std::size_t c, const QVariant& v);
	bool iat(std::size_t r, std::size_t c, const pybind11::object& v);
	// DataFrame.loc 获取一行
	DAPySeries iloc(std::size_t c) const;
	DAPySeries loc(const QString& n) const;
	bool drop(std::size_t index, int axis = 0);
	DAPyIndex index() const;
	// 获取某一列对应的dtype
	pybind11::dtype dtypes(std::size_t c) const;
	DAPyDataFrame describe() const;
    // 转换为csv
    bool to_csv(const QString& path, const QVariantHash& args = QVariantHash()) const noexcept;
    // 转换为pickle
    bool to_pickle(const QString& path, const QVariantHash& args = QVariantHash()) const noexcept;
    // 转换为parquet
    bool to_parquet(const QString& path, const QVariantHash& args = QVariantHash()) const noexcept;

protected:
	// 检测是否为dataframe，如果不是将会设置为none
	void checkObjectValid();

public:
	QString toString(std::size_t maxrow = 12) const;
	static bool isDataFrame(const pybind11::object& obj);

public:
	bool read_csv(const QString& path, const QVariantHash args = QVariantHash(), QString* why = nullptr);
};
}  // namespace DA
DAPYBINDQT_API QDebug operator<<(QDebug dbg, const DA::DAPyDataFrame& df);

Q_DECLARE_METATYPE(DA::DAPyDataFrame)
#endif  // DADATAFRAME_H
