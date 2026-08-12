#include "DAPyIndex.h"
#include "DAPyModulePandas.h"
#include "DAPybind11QtCaster.hpp"
#include "DALogCategory.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyIndex
//===================================================
/**
 * @brief 构造一个空的pandas.Index对象
 */
DAPyIndex::DAPyIndex() : DAPyObjectWrapper()
{
    try {
        auto pandas = DAPyModule("pandas");
        object()     = pandas.attr("Index");
    } catch (const std::exception& e) {
        qCritical() << "can not import pandas,or can not create pandas.Index(),because:" << e.what();
    }
}

/**
 * @brief 拷贝构造
 * @param s 另一个DAPyIndex
 */
DAPyIndex::DAPyIndex(const DAPyIndex& s) : DAPyObjectWrapper(s)
{
    checkObjectValid();
}

/**
 * @brief 移动构造
 * @param s 右值DAPyIndex
 */
DAPyIndex::DAPyIndex(DAPyIndex&& s) : DAPyObjectWrapper(std::move(s))
{
    checkObjectValid();
}

/**
 * @brief 构造，从pybind11::object构造
 * @param obj pybind11对象
 */
DAPyIndex::DAPyIndex(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
    checkObjectValid();
}

/**
 * @brief 构造，从pybind11::object右值构造
 * @param obj pybind11对象右值
 */
DAPyIndex::DAPyIndex(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
    checkObjectValid();
}

/**
 * @brief 析构
 */
DAPyIndex::~DAPyIndex()
{
}

/**
 * @brief 判断对象是否为pandas.Index实例
 * @param obj 要判断的对象
 * @return 如果是Index返回true
 */
bool DAPyIndex::isIndexObj(const pybind11::object& obj)
{
    return DAPyModulePandas::getInstance().isInstanceIndex(obj);
}

/**
 * @brief 赋值操作符
 * @param obj pybind11对象
 * @return 返回自身的引用
 */
DAPyIndex& DAPyIndex::operator=(const pybind11::object& obj)
{
    object() = obj;
    checkObjectValid();
    return *this;
}

/**
 * @brief 移动赋值操作符
 * @param obj pybind11对象右值
 * @return 返回自身的引用
 */
DAPyIndex& DAPyIndex::operator=(pybind11::object&& obj)
{
    object() = std::move(obj);
    checkObjectValid();
    return *this;
}

/**
 * @brief 拷贝赋值操作符
 * @param obj 另一个DAPyIndex
 * @return 返回自身的引用
 */
DAPyIndex& DAPyIndex::operator=(const DAPyIndex& obj)
{
    if (this != &obj) {
        DAPyObjectWrapper::operator=(obj);  // 调用基类赋值
        checkObjectValid();
    }
    return *this;
}

/**
 * @brief 移动赋值操作符
 * @param obj 右值DAPyIndex
 * @return 返回自身的引用
 */
DAPyIndex& DAPyIndex::operator=(DAPyIndex&& obj)
{
    if (this != &obj) {
        DAPyObjectWrapper::operator=(std::move(obj));  // 调用基类移动赋值
        checkObjectValid();
    }
    return *this;
}

/**
 * @brief 赋值操作符，从DAPyObjectWrapper赋值
 * @param obj DAPyObjectWrapper对象
 * @return 返回自身的引用
 */
DAPyIndex& DAPyIndex::operator=(const DAPyObjectWrapper& obj)
{
    DAPyObjectWrapper::operator=(obj);
    checkObjectValid();
    return *this;
}

/**
 * @brief 移动赋值操作符，从DAPyObjectWrapper移动赋值
 * @param obj DAPyObjectWrapper对象右值
 * @return 返回自身的引用
 */
DAPyIndex& DAPyIndex::operator=(DAPyObjectWrapper&& obj)
{
    DAPyObjectWrapper::operator=(std::move(obj));
    checkObjectValid();
    return *this;
}

/**
 * @brief 通过位置索引获取元素
 * @param i 位置索引
 * @return 对应位置的pybind11::object
 */
pybind11::object DAPyIndex::operator[](std::size_t i) const
{
    return object()[ pybind11::int_(i) ];
}

/**
 * @brief 通过位置索引获取元素
 * @param i 位置索引
 * @return 对应位置的pybind11::object
 */
pybind11::object DAPyIndex::iat(size_t i) const
{
    return object()[ pybind11::int_(i) ];
}

/**
 * @brief 通过位置索引集合获取元素
 * @param slice 位置索引集合
 * @return 对应位置的pybind11::object
 */
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

/**
 * @brief 转换为pybind11::list
 * @return 转换后的pybind11::list
 */
pybind11::list DAPyIndex::toList() const
{
    return attr("tolist")();
}

/**
 * @brief 判断索引是否为空
 * @return 如果为空返回true
 */
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

/**
 * @brief 获取索引的元素数量
 * @return 返回元素数量
 */
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

/**
 * @brief 获取指定位置的元素值
 * @param i 位置索引
 * @return 返回QVariant形式的值
 */
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
 * @brief 获取 label 的精确位置索引
 *
 * 基于 pandas Index.get_loc 实现。get_loc 对唯一 label 返回 int 位置；
 * 对单调索引中的重复连续 label 返回 slice；对非单调索引的重复 label 返回 boolean mask。
 * 本函数仅处理"唯一 label 返回 int"的主路径，其余情况返回 -2 由调用方回退处理。
 * @param label 要查找的 index label
 * @return label 唯一时返回位置（>=0）；label 重复返回 -2；异常/不存在返回 -1
 */
long DAPyIndex::getLoc(const pybind11::object& label) const
{
    try {
        pybind11::object loc = attr("get_loc")(label);
        if (pybind11::isinstance< pybind11::int_ >(loc)) {
            return loc.cast< long >();
        }
        // slice 或 boolean mask：label 重复
        return -2;
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
        return -1;
    }
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
int64_t DAPyIndex::getIndexer(pybind11::object v, const char* method)
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
            return res[ 0 ].cast< int64_t >();
        }
        return -1;
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return -1;
}

/**
 * @brief 检查对象是否为有效的Index，无效则置为None
 */
void DAPyIndex::checkObjectValid()
{
    if (!isIndexObj(object())) {
        object() = pybind11::none();
        daCritical << QObject::tr(
            "DAPyIndex: the Python object type is not pandas.Index");  // cn:DAPyIndex：Python 对象类型不是 pandas.Index
    }
}
