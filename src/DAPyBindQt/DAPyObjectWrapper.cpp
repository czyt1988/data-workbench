#include "DAPyObjectWrapper.h"
#include "DAPybind11QtCaster.hpp"
#include <QDebug>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyObjectWrapper
//===================================================

/**
 * @brief 构造一个DAPyObjectWrapper对象
 *
 * 默认构造函数，将内部Python对象初始化为None，
 * 并设置默认的错误回调函数（通过qCritical输出错误信息）。
 */
DAPyObjectWrapper::DAPyObjectWrapper()
    : mObject(pybind11::none()), mErrCallback([](const char* e) { qCritical() << e; })
{
}

/**
 * @brief 拷贝构造函数
 * @param obj 要拷贝的DAPyObjectWrapper对象
 */
DAPyObjectWrapper::DAPyObjectWrapper(const DAPyObjectWrapper& obj)
    : mObject(obj.mObject), mErrCallback(obj.mErrCallback)
{
}

/**
 * @brief 移动构造函数
 * @param obj 要移动的DAPyObjectWrapper对象
 */
DAPyObjectWrapper::DAPyObjectWrapper(DAPyObjectWrapper&& obj)
    : mObject(std::move(obj.mObject)), mErrCallback(std::move(obj.mErrCallback))
{
}

/**
 * @brief 通过pybind11::object构造DAPyObjectWrapper对象
 * @param obj Python对象
 */
DAPyObjectWrapper::DAPyObjectWrapper(const pybind11::object& obj)
    : mObject(obj), mErrCallback([](const char* e) { qCritical() << e; })
{
}

/**
 * @brief 通过pybind11::object右值构造DAPyObjectWrapper对象
 * @param obj Python对象右值
 */
DAPyObjectWrapper::DAPyObjectWrapper(pybind11::object&& obj)
    : mObject(std::move(obj)), mErrCallback([](const char* e) { qCritical() << e; })
{
}

/**
 * @brief 析构DAPyObjectWrapper，释放Python对象引用
 */
DAPyObjectWrapper::~DAPyObjectWrapper()
{
    if (!Py_IsInitialized()) {
        mObject.release();
    }
}

/**
 * @brief 判断内部Python对象是否为None
 * @return 如果为None返回true，否则返回false
 */
bool DAPyObjectWrapper::isNone() const
{
    return mObject.is_none();
}

/**
 * @brief 拷贝赋值操作符
 * @param obj 要拷贝的DAPyObjectWrapper对象
 * @return 返回自身的引用
 */
DAPyObjectWrapper& DAPyObjectWrapper::operator=(const DAPyObjectWrapper& obj)
{
    if (this != &obj) {
        mObject      = obj.mObject;
        mErrCallback = obj.mErrCallback;
    }
    return *this;
}

/**
 * @brief 移动赋值操作符
 * @param obj 要移动的DAPyObjectWrapper对象
 * @return 返回自身的引用
 */
DAPyObjectWrapper& DAPyObjectWrapper::operator=(DAPyObjectWrapper&& obj)
{
    if (this != &obj) {
        mObject      = std::move(obj.mObject);
        mErrCallback = std::move(obj.mErrCallback);
    }
    return *this;
}

/**
 * @brief 通过pybind11::object赋值
 * @param obj Python对象
 * @return 返回自身的引用
 */
DAPyObjectWrapper& DAPyObjectWrapper::operator=(const pybind11::object& obj)
{
    mObject = obj;
    return *this;
}

/**
 * @brief 通过pybind11::object右值赋值
 * @param obj Python对象右值
 * @return 返回自身的引用
 */
DAPyObjectWrapper& DAPyObjectWrapper::operator=(pybind11::object&& obj)
{
    mObject = std::move(obj);
    return *this;
}

/**
 * @brief 判断内部Python对象指针是否等于给定指针
 * @param ptr 要比较的指针
 * @return 如果相等返回true
 */
bool DAPyObjectWrapper::operator==(void* ptr) const
{
    return (mObject.ptr() == ptr);
}

/**
 * @brief 判断内部Python对象是否与给定对象相同（身份比较）
 * @param obj 要比较的Python对象
 * @return 如果是同一个对象返回true
 */
bool DAPyObjectWrapper::operator==(const pybind11::object& obj) const
{
    return mObject.is(obj);
}

/**
 * @brief 判断两个DAPyObjectWrapper是否包装了同一个Python对象
 * @param obj 要比较的DAPyObjectWrapper对象
 * @return 如果是同一个对象返回true
 */
bool DAPyObjectWrapper::operator==(const DAPyObjectWrapper& obj) const
{
    return mObject.is(obj.mObject);
}

/**
 * @brief bool操作符，判断内部Python对象是否不为None
 * @return 如果不为None返回true
 */
DAPyObjectWrapper::operator bool() const
{
    return !isNone();
}

/**
 * @brief 统一的异常处理函数
 *
 * 此函数会调用赋予的回调函数处理异常信息
 * @param e 异常对象
 */
void DAPyObjectWrapper::dealException(const std::exception& e) const
{
    if (mErrCallback) {
        mErrCallback(e.what());
    }
}

/**
 * @brief 深拷贝当前Python对象
 *
 * 使用Python的copy.deepcopy进行深拷贝，
 * 如果Python未初始化或对象为None，返回空的DAPyObjectWrapper。
 * @return 深拷贝后的DAPyObjectWrapper对象
 */
DAPyObjectWrapper DAPyObjectWrapper::deepCopy() const
{
    if (isNone()) {
        return DAPyObjectWrapper();
    }
    if (!Py_IsInitialized()) {
        return DAPyObjectWrapper();
    }

    try {
        pybind11::object copy_mod = pybind11::module::import("copy");
        pybind11::object copied   = copy_mod.attr("deepcopy")(mObject);
        return DAPyObjectWrapper(copied);
    } catch (const std::exception& e) {
        dealException(e);
        return DAPyObjectWrapper();
    }
}

/**
 * @brief 将Python对象转换为QVariant
 * @return 转换后的QVariant
 */
QVariant DAPyObjectWrapper::toVariant() const
{
    return object().cast< QVariant >();
}

/**
 * @brief 判断内部Python对象是否为指定类型的实例
 * @param type Python类型对象
 * @return 如果是指定类型的实例返回true
 */
bool DAPyObjectWrapper::isinstance(const pybind11::handle& type) const
{
    return pybind11::isinstance(mObject, type);
}

/**
 * @brief 判断是否为int类型
 * @return 如果是int类型返回true
 */
bool DAPyObjectWrapper::isInt() const
{
    return pybind11::isinstance< pybind11::int_ >(mObject);
}

/**
 * @brief 判断是否为Module类型
 * @return 如果是Module类型返回true
 */
bool DAPyObjectWrapper::isModule() const
{
    return pybind11::isinstance< pybind11::module_ >(mObject);
}

/**
 * @brief 判断是否为float类型
 * @return 如果是float类型返回true
 */
bool DAPyObjectWrapper::isFloat() const
{
    return pybind11::isinstance< pybind11::float_ >(mObject);
}

/**
 * @brief 判断是否为str类型
 * @return 如果是str类型返回true
 */
bool DAPyObjectWrapper::isStr() const
{
    return pybind11::isinstance< pybind11::str >(mObject);
}

/**
 * @brief 判断是否为bool类型
 * @return 如果是bool类型返回true
 */
bool DAPyObjectWrapper::isBool() const
{
    return pybind11::isinstance< pybind11::bool_ >(mObject);
}

/**
 * @brief 判断是否为list类型
 * @return 如果是list类型返回true
 */
bool DAPyObjectWrapper::isList() const
{
    return pybind11::isinstance< pybind11::list >(mObject);
}

/**
 * @brief 判断是否为dict类型
 * @return 如果是dict类型返回true
 */
bool DAPyObjectWrapper::isDict() const
{
    return pybind11::isinstance< pybind11::dict >(mObject);
}

/**
 * @brief 判断是否为tuple类型
 * @return 如果是tuple类型返回true
 */
bool DAPyObjectWrapper::isTuple() const
{
    return pybind11::isinstance< pybind11::tuple >(mObject);
}

/**
 * @brief 判断是否为可调用对象
 * @return 如果是可调用对象返回true
 */
bool DAPyObjectWrapper::isCallable() const
{
    if (isNone()) {
        return false;
    }
    return pybind11::isinstance< pybind11::function >(mObject) || PyCallable_Check(mObject.ptr());
}

/**
 * @brief 判断是否为序列类型
 * @return 如果是序列类型返回true
 */
bool DAPyObjectWrapper::isSequence() const
{
    if (isNone()) {
        return false;
    }
    return PySequence_Check(mObject.ptr());
}

/**
 * @brief 判断是否为数字类型
 * @return 如果是数字类型返回true
 */
bool DAPyObjectWrapper::isNumeric() const
{
    if (isNone()) {
        return false;
    }
    return PyNumber_Check(mObject.ptr()) != 0;
}

/**
 * @brief 设置错误处理回调函数
 * @param fun 回调函数
 */
void DAPyObjectWrapper::setErrCallback(const DAPyObjectWrapper::ErrCallback& fun)
{
    mErrCallback = fun;
}

/**
 * @brief 获取错误处理回调函数
 * @return 当前的错误处理回调函数
 */
DAPyObjectWrapper::ErrCallback DAPyObjectWrapper::getErrCallback() const
{
    return mErrCallback;
}

/**
 * @brief 获取Python对象的属性
 * @param c_att 属性名
 * @return 属性对应的Python对象
 */
pybind11::object DAPyObjectWrapper::attr(const char* c_att)
{
    return mObject.attr(c_att);
}

/**
 * @brief 获取Python对象的属性（const版本）
 * @param c_att 属性名
 * @return 属性对应的Python对象
 */
pybind11::object DAPyObjectWrapper::attr(const char* c_att) const
{
    return mObject.attr(c_att);
}

/**
 * @brief 判断Python对象是否有指定属性
 * @param c_att 属性名
 * @return 如果存在该属性返回true
 */
bool DAPyObjectWrapper::hasattr(const char* c_att) const
{
    return pybind11::hasattr(mObject, c_att);
}

/**
 * @brief 获取Python对象的__name__属性
 * @return 对象名称字符串，失败返回空字符串
 */
QString DAPyObjectWrapper::__name__() const
{
    if (isNone()) {
        return QString();
    }
    try {
        pybind11::str n = mObject.attr("__name__");
        return pybind11::cast< QString >(n);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 获取Python对象的字符串表示（__str__）
 * @return 字符串表示，失败返回空字符串
 */
QString DAPyObjectWrapper::__str__() const
{
    if (isNone()) {
        return QString();
    }
    try {
        return pybind11::cast< QString >(pybind11::str(mObject));
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 获取Python对象的表达式表示（__repr__）
 * @return 表达式表示字符串，失败返回空字符串
 */
QString DAPyObjectWrapper::__repr__() const
{
    if (isNone()) {
        return QString();
    }
    try {
        return pybind11::cast< QString >(pybind11::repr(mObject));
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 获取Python对象的类型名称
 * @return 类型名称字符串，失败返回"none"
 */
QString DAPyObjectWrapper::typeName() const
{
    if (isNone()) {
        return QString("none");
    }
    try {
        return pybind11::cast< QString >(pybind11::str(pybind11::type::handle_of(mObject).attr("__name__")));
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 获取Python对象的引用计数
 * @return 引用计数，如果对象为None返回0
 */
size_t DAPyObjectWrapper::refCount() const
{
    if (isNone()) {
        return 0;
    }
    return mObject.ref_count();
}

/**
 * @brief 小于比较操作符，基于Python对象指针地址排序
 *
 * 提供严格弱序关系，使DAPyObjectWrapper及其派生类可用作QMap的key。
 * 比较基于PyObject*指针地址，语义与operator==（身份比较）一致。
 *
 * @param obj 比较目标
 * @return 如果当前对象指针地址小于目标返回true
 */
bool DAPyObjectWrapper::operator<(const DAPyObjectWrapper& obj) const
{
    return std::less<PyObject*>()(mObject.ptr(), obj.mObject.ptr());
}

/**
 * @brief Qt哈希函数，基于Python对象指针地址生成哈希值
 *
 * 使DAPyObjectWrapper及其派生类可用作QHash的key。
 *
 * @param obj 要哈希的对象
 * @param seed 哈希种子
 * @return 哈希值
 */
uint qHash(const DAPyObjectWrapper& obj, uint seed)
{
    return static_cast< uint >(::qHash(reinterpret_cast<quintptr>(obj.object().ptr()), seed));
}

/**
 * @brief std::hash特化，基于Python对象指针地址生成哈希值
 * @param obj 要哈希的对象
 * @return 哈希值
 */
namespace std
{
size_t hash<DA::DAPyObjectWrapper>::operator()(const DA::DAPyObjectWrapper& obj) const noexcept
{
    return std::hash<PyObject*>()(obj.object().ptr());
}
}  // namespace std
