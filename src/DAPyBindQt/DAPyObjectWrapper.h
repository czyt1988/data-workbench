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
    DAPyObjectWrapper& operator=(const pybind11::object& obj);
    bool operator==(void* ptr) const;
    bool operator==(const pybind11::object& obj) const;
    bool operator==(const DAPyObjectWrapper& obj) const;
    // bool操作符可直接进行isNone判断
    explicit operator bool() const;
    // 统一异常处理函数
    void dealException(const std::exception& e) const;

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
    bool isInt() const;
    bool isModule() const;
    bool isFloat() const;
    bool isStr() const;
    // 设置错误处理回调
    void setErrCallback(const ErrCallback& fun);
    ErrCallback getErrCallback() const;

public:
    // 重载
    pybind11::object attr(const char* c_att);
    pybind11::object attr(const char* c_att) const;

public:
    // 通用的python函数封装
    QString __name__() const;

protected:
    pybind11::object _object;
    ErrCallback _errcallback;
};
}  // namespace DA
Q_DECLARE_METATYPE(DA::DAPyObjectWrapper)
#endif  // DAPYOBJECTWRAPPER_H
