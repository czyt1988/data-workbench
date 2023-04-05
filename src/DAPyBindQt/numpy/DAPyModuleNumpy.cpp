#include "DAPyModuleNumpy.h"
#include <QDebug>
#include <QObject>
namespace DA
{
class DAPyModuleNumpyPrivate
{
public:
    DA_IMPL_PUBLIC(DAPyModuleNumpy)
    DAPyModuleNumpyPrivate(DAPyModuleNumpy* p);

    //释放模块
    void del();

public:
    QString _lastErrorString;
    pybind11::object _objDtype;
    pybind11::object _objGeneric;
    pybind11::object _objNumber;
    pybind11::object _objInteger;
    pybind11::object _objInexact;
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyModuleNumpyPrivate
//===================================================

DAPyModuleNumpyPrivate::DAPyModuleNumpyPrivate(DAPyModuleNumpy* p) : q_ptr(p)
{
}

void DAPyModuleNumpyPrivate::del()
{
    if (!q_ptr->isImport()) {
        return;
    }
    q_ptr->object() = pybind11::none();
}

DAPyModuleNumpy::DAPyModuleNumpy() : DAPyModule(), d_ptr(new DAPyModuleNumpyPrivate(this))
{
    import();
    try {
        d_ptr->_objDtype   = attr("dtype");
        d_ptr->_objGeneric = attr("generic");
        d_ptr->_objNumber  = attr("number");
        d_ptr->_objInteger = attr("integer");
        d_ptr->_objInexact = attr("inexact");
    } catch (const std::exception& e) {
        d_ptr->_lastErrorString = e.what();
    }
}

DAPyModuleNumpy::~DAPyModuleNumpy()
{
}

DAPyModuleNumpy& DAPyModuleNumpy::getInstance()
{
    static DAPyModuleNumpy s_numpy;
    return s_numpy;
}

void DAPyModuleNumpy::finalize()
{
    d_ptr->del();
}

QString DAPyModuleNumpy::getLastErrorString()
{
    return d_ptr->_lastErrorString;
}

/**
 * @brief 判断是否为numpy.generic
 * @param obj
 * @return
 */
bool DAPyModuleNumpy::isInstanceGeneric(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_objGeneric);
}

bool DAPyModuleNumpy::isInstanceNumber(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_objNumber);
}

bool DAPyModuleNumpy::isInstanceInteger(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_objInteger);
}

bool DAPyModuleNumpy::isInstanceInexact(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_objInexact);
}

bool DAPyModuleNumpy::isInstanceDtype(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_objDtype);
}

bool DAPyModuleNumpy::import()
{
    return DAPyModule::import("numpy");
}
