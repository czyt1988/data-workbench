#ifndef DAPYOBJECTWRAPPER_H
#define DAPYOBJECTWRAPPER_H
#include "DAPyBindQtGlobal.h"
#include "DAPybind11InQt.h"
#include <QVariant>
#include <functional>

namespace DA
{
/**
 * @brief 这是针对pubind11::object的封装
 */

class DAPYBINDQT_API DAPyObjectWrapper
{
public:
    /**
     * @brief 异常错误回调函数
     */
    using ErrCallback = std::function< void(const char*) >;

public:
    DAPyObjectWrapper();
    DAPyObjectWrapper(const DAPyObjectWrapper& obj);
    DAPyObjectWrapper(DAPyObjectWrapper&& obj);
    DAPyObjectWrapper(const pybind11::object& obj);
    DAPyObjectWrapper(pybind11::object&& obj);
    virtual ~DAPyObjectWrapper();
    // 判断是否为none
    bool isNone() const;
    // 操作符
    DAPyObjectWrapper& operator=(const DAPyObjectWrapper& obj);
    DAPyObjectWrapper& operator=(DAPyObjectWrapper&& obj);
    DAPyObjectWrapper& operator=(const pybind11::object& obj);
    DAPyObjectWrapper& operator=(pybind11::object&& obj);
    // 比较操作符
    bool operator==(void* ptr) const;
    bool operator==(const pybind11::object& obj) const;
    bool operator==(const DAPyObjectWrapper& obj) const;
    bool operator!=(void* ptr) const
    {
        return !(*this == ptr);
    }
    bool operator!=(const pybind11::object& obj) const
    {
        return !(*this == obj);
    }
    bool operator!=(const DAPyObjectWrapper& obj) const
    {
        return !(*this == obj);
    }
    // bool操作符可直接进行isNone判断
    explicit operator bool() const;
    // 统一异常处理函数
    void dealException(const std::exception& e) const;
    // 深拷贝
    DAPyObjectWrapper deepCopy() const;

public:
    pybind11::object& object()
    {
        return _object;
    }
    const pybind11::object& object() const
    {
        return _object;
    }

public:
    // 转换为QVariant
    QVariant toVariant() const;
    // 判断类型
    bool isinstance(const pybind11::handle& type) const;
    bool isInt() const;
    bool isModule() const;
    bool isFloat() const;
    bool isStr() const;
    bool isBool() const;
    bool isList() const;
    bool isDict() const;
    bool isTuple() const;
    bool isCallable() const;
    bool isSequence() const;
    bool isNumeric() const;
    // 设置错误处理回调
    void setErrCallback(const ErrCallback& fun);
    ErrCallback getErrCallback() const;

public:
    // 重载
    pybind11::object attr(const char* c_att);
    pybind11::object attr(const char* c_att) const;
    // 方法调用
    template< typename... Args >
    pybind11::object call(Args&&... args);

public:
    // 通用的python函数封装
    QString __name__() const;
    QString __str__() const;
    QString __repr__() const;
    // 对象信息
    QString typeName() const;
    size_t refCount() const;

protected:
    pybind11::object _object;
    ErrCallback _errcallback;
};

/**
 * @brief 直接调用Python可调用对象
 *
 * 此函数用于调用当前包装的Python可调用对象（如函数、方法、lambda等）。
 * 如果对象不可调用，将抛出 std::runtime_error 异常。
 * 如果Python调用过程中发生异常，将通过pybind11抛出 pybind11::error_already_set 异常。
 *
 * @tparam Args 参数类型包，自动推导
 * @param args 调用参数，支持任意数量和类型的参数
 * @return pybind11::object Python调用返回的对象
 *
 * @throws std::runtime_error 如果对象不可调用
 * @throws pybind11::error_already_set 如果Python调用过程中发生异常
 *
 * @note 此函数不捕获任何异常，调用者需要自行处理可能的异常
 *
 * @code
 * // 示例1：调用无参数函数
 * DAPyObjectWrapper func = ...; // 获取Python函数
 * pybind11::object result = func.call();
 *
 * // 示例2：调用带参数函数
 * DAPyObjectWrapper func = ...; // 获取Python函数
 * pybind11::object result = func.call(42, "hello", 3.14);
 *
 * // 示例3：调用方法并处理异常
 * try {
 *     DAPyObjectWrapper obj = ...; // 获取Python对象
 *     pybind11::object result = obj.call(x, y);
 *     // 处理结果...
 * } catch (const pybind11::error_already_set& e) {
 *     qCritical() << "Python call failed:" << e.what();
 * } catch (const std::runtime_error& e) {
 *     qCritical() << "Object is not callable:" << e.what();
 * }
 * @endcode
 */
template< typename... Args >
pybind11::object DAPyObjectWrapper::call(Args&&... args)
{
    if (!isCallable()) {
        return pybind11::none();
    }
    return _object(std::forward< Args >(args)...);
}

}  // namespace DA
Q_DECLARE_METATYPE(DA::DAPyObjectWrapper)
#endif  // DAPYOBJECTWRAPPER_H
