#include "DAPyModuleNumpy.h"
#include <QDebug>
#include <QObject>
namespace DA
{
class DAPyModuleNumpy::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyModuleNumpy)
public:
    PrivateData(DAPyModuleNumpy* p);

    //释放模块
    void del();

public:
    QString mLastErrorString;
    pybind11::object mObjDtype;
    pybind11::object mObjGeneric;
    pybind11::object mObjNumber;
    pybind11::object mObjInteger;
    pybind11::object mObjInexact;
};

//===================================================
// DAPyModuleNumpyPrivate
//===================================================

DAPyModuleNumpy::PrivateData::PrivateData(DAPyModuleNumpy* p) : q_ptr(p)
{
}

void DAPyModuleNumpy::PrivateData::del()
{
    if (!q_ptr->isImport()) {
        return;
    }
    q_ptr->object() = pybind11::none();
}

//===================================================
// DAPyModuleNumpy
//===================================================
DAPyModuleNumpy::DAPyModuleNumpy() : DAPyModule(), DA_PIMPL_CONSTRUCT
{
    import();
    try {
        d_ptr->mObjDtype   = attr("dtype");
        d_ptr->mObjGeneric = attr("generic");
        d_ptr->mObjNumber  = attr("number");
        d_ptr->mObjInteger = attr("integer");
        d_ptr->mObjInexact = attr("inexact");
    } catch (const std::exception& e) {
        d_ptr->mLastErrorString = e.what();
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
    return d_ptr->mLastErrorString;
}

/**
 * @brief 判断是否为numpy.generic
 * @param obj
 * @return
 */
bool DAPyModuleNumpy::isInstanceGeneric(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjGeneric);
}

bool DAPyModuleNumpy::isInstanceNumber(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjNumber);
}

bool DAPyModuleNumpy::isInstanceInteger(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjInteger);
}

bool DAPyModuleNumpy::isInstanceInexact(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjInexact);
}

bool DAPyModuleNumpy::isInstanceDtype(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjDtype);
}

bool DAPyModuleNumpy::import()
{
    return DAPyModule::import("numpy");
}
}  // namespace DA
