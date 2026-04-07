#ifndef DAPYDATAFRAME_H
#define DAPYDATAFRAME_H
#include "DAPyBindQtGlobal.h"
#include "DAPyObjectWrapper.h"
#include "DAPyIndex.h"
#include "DAPybind11InQt.h"
#include "DAPySeries.h"
#include "numpy/DAPyDType.h"
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

    DAPySeries operator[](const QString& n) const;
    DAPySeries operator[](size_t n) const;
    DAPyDataFrame& operator=(const pybind11::object& obj);
    DAPyDataFrame& operator=(const DAPyDataFrame& obj);
    DAPyDataFrame& operator=(const DAPyObjectWrapper& obj);
    DAPyDataFrame& operator=(pybind11::object&& obj);
    DAPyDataFrame& operator=(DAPyDataFrame&& obj);
    DAPyDataFrame& operator=(DAPyObjectWrapper&& obj);
    bool operator<(const DAPyDataFrame& other) const;

public:
    QList< QString > columns() const;
    QString columnName(std::size_t i) const;
    bool columns(std::size_t i, const QString& name);
    bool columns(const QList< QString >& cols);
    bool empty() const;
    std::pair< std::size_t, std::size_t > shape() const;
    std::size_t size() const;
    QVariant iat(std::size_t r, std::size_t c) const;
    pybind11::object iatObj(std::size_t r, std::size_t c) const;
    bool iat(std::size_t r, std::size_t c, const QVariant& v);
    bool iat(std::size_t r, std::size_t c, const pybind11::object& v);
    DAPySeries iloc(std::size_t c) const;
    DAPySeries loc(const QString& n) const;
    bool drop(std::size_t index, int axis = 0);
    DAPyIndex index() const;
    pybind11::dtype dtypes(std::size_t c) const;
    DAPyDType dtypeObject(std::size_t c) const;
    DAPyDataFrame describe() const;
    bool to_csv(const QString& path, const QVariantHash& args = QVariantHash()) const noexcept;
    bool to_pickle(const QString& path, const QVariantHash& args = QVariantHash()) const noexcept;
    bool to_parquet(const QString& path, const QVariantHash& args = QVariantHash()) const noexcept;
    bool setIndex(const QString& colName, bool isDrop = true) noexcept;

protected:
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
