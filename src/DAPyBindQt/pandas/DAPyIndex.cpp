#include "DAPyIndex.h"
#include "DAPyModulePandas.h"
#include "DAPybind11QtCaster.hpp"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyIndex
//===================================================
DAPyIndex::DAPyIndex() : DAPyObjectWrapper()
{
    try {
        auto pandas = DAPyModule("pandas");
        _object     = pandas.attr("Index");
    } catch (const std::exception& e) {
        qCritical() << "can not import pandas,or can not create pandas.Index(),because:" << e.what();
    }
}

DAPyIndex::DAPyIndex(const DAPyIndex& s) : DAPyObjectWrapper(s)
{
    checkObjectValid();
}

DAPyIndex::DAPyIndex(DAPyIndex&& s) : DAPyObjectWrapper(std::move(s))
{
    checkObjectValid();
}

DAPyIndex::DAPyIndex(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
    checkObjectValid();
}

DAPyIndex::DAPyIndex(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
    checkObjectValid();
}

DAPyIndex::~DAPyIndex()
{
}

bool DAPyIndex::isIndexObj(const pybind11::object& obj)
{
    return DAPyModulePandas::getInstance().isInstanceIndex(obj);
}

DAPyIndex& DAPyIndex::operator=(const pybind11::object& obj)
{
    _object = obj;
    checkObjectValid();
    return *this;
}

DAPyIndex& DAPyIndex::operator=(pybind11::object&& obj)
{
    _object = std::move(obj);
    checkObjectValid();
    return *this;
}

DAPyIndex& DAPyIndex::operator=(const DAPyIndex& obj)
{
    if (this != &obj) {
        DAPyObjectWrapper::operator=(obj);  // 调用基类赋值
        checkObjectValid();
    }
    return *this;
}

DAPyIndex& DAPyIndex::operator=(DAPyIndex&& obj)
{
    if (this != &obj) {
        DAPyObjectWrapper::operator=(std::move(obj));  // 调用基类移动赋值
        checkObjectValid();
    }
    return *this;
}

DAPyIndex& DAPyIndex::operator=(const DAPyObjectWrapper& obj)
{
    DAPyObjectWrapper::operator=(obj);
    checkObjectValid();
    return *this;
}

DAPyIndex& DAPyIndex::operator=(DAPyObjectWrapper&& obj)
{
    DAPyObjectWrapper::operator=(std::move(obj));
    checkObjectValid();
    return *this;
}

pybind11::object DAPyIndex::operator[](std::size_t i) const
{
    return object()[ pybind11::int_(i) ];
}

pybind11::object DAPyIndex::iat(size_t i) const
{
    return object()[ pybind11::int_(i) ];
}

pybind11::object DAPyIndex::operator[](const QSet< std::size_t >& slice) const
{
    try {
        return object()[ DA::PY::toPyObject(slice) ];
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return pybind11::none();
}

/**
 * @brief Return the dtype object of the underlying data.
 * @return
 */
pybind11::dtype DAPyIndex::dtype() const
{
    try {
        return attr("dtype");
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return pybind11::none();
}

pybind11::list DAPyIndex::toList() const
{
    return attr("tolist")();
}

bool DAPyIndex::empty() const
{
    try {
        pybind11::bool_ obj = attr("empty");
        return obj.cast< bool >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return true;
}

std::size_t DAPyIndex::size() const
{
    try {
        pybind11::int_ obj = attr("size");
        return std::size_t(obj);
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return 0;
}

QVariant DAPyIndex::value(size_t i) const
{
    try {
        return iat(i).cast< QVariant >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return QVariant();
}

/**
 * @brief 获取索引号，返回-1代表获取失败
 * @param v
 * @param method
 * 'pad' / 'ffill' 向前填充：找 ≤ 查询值 的最大索引
 * 'backfill' / 'bfill' 向后填充：找 ≥ 查询值 的最小索引
 * 'nearest' 最近邻：前后均可，绝对值最小
 * none 等同 None
 * @return
 */
uint64_t DAPyIndex::getIndexer(pybind11::object v, const char* method)
{
    try {
        pybind11::object methodObj = pybind11::none();
        if (std::strcmp(method, "none") != 0) {
            // 不是none
            methodObj = pybind11::cast(method);
        }
        pybind11::list list;
        list.append(v);
        pybind11::list res = attr("get_indexer")(list, pybind11::arg("method") = methodObj);
        if (res.size() > 0) {
            return res[ 0 ].cast< uint64_t >();
        }
        return -1;
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return -1;
}

void DAPyIndex::checkObjectValid()
{
    if (!isIndexObj(object())) {
        object() = pybind11::none();
        qCritical() << QObject::tr("DAPyIndex get python object type is not pandas.Index");
    }
}
