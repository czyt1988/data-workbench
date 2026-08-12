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
    // 异常错误回调函数
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
    // 比较操作符（用于QMap等有序容器的key）
    bool operator<(const DAPyObjectWrapper& obj) const;

    // bool操作符可直接进行isNone判断,isNone返回false
    explicit operator bool() const;
    // 统一异常处理函数
    void dealException(const std::exception& e) const;
    // 深拷贝
    DAPyObjectWrapper deepCopy() const;

public:
    pybind11::object& object()
    {
        return mObject;
    }
    const pybind11::object& object() const
    {
        return mObject;
    }
    pybind11::handle& handle()
    {
        return mObject;
    }
    const pybind11::handle& handle() const
    {
        return mObject;
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
    // hasattr
    bool hasattr(const char* c_att) const;

public:
    // 通用的python函数封装
    QString __name__() const;
    QString __str__() const;
    QString __repr__() const;
    // 对象信息
    QString typeName() const;
    size_t refCount() const;

protected:
    pybind11::object mObject;
    ErrCallback mErrCallback;
};

/**
 * @brief 直接调用Python可调用对象
 *
 * @tparam Args 参数类型包，自动推导
 * @param args 调用参数，支持任意数量和类型的参数
 * @return pybind11::object Python调用返回的对象
 */
template< typename... Args >
pybind11::object DAPyObjectWrapper::call(Args&&... args)
{
    if (!isCallable()) {
        return pybind11::none();
    }
    return mObject(std::forward< Args >(args)...);
}

// qHash自由函数（用于QHash容器的key）
DAPYBINDQT_API uint qHash(const DAPyObjectWrapper& obj, uint seed = 0);

}  // namespace DA

Q_DECLARE_METATYPE(DA::DAPyObjectWrapper)

// std::hash特化（用于std::unordered_map的key）
namespace std
{
template<>
struct hash< DA::DAPyObjectWrapper >
{
    size_t operator()(const DA::DAPyObjectWrapper& obj) const noexcept;
};
}  // namespace std

#endif  // DAPYOBJECTWRAPPER_H
