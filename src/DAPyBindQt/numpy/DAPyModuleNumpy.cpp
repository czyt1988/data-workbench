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

/**
 * @brief 析构
 */
DAPyModuleNumpy::~DAPyModuleNumpy()
{
}

/**
 * @brief 获取单例实例
 * @return 返回DAPyModuleNumpy单例引用
 */
DAPyModuleNumpy& DAPyModuleNumpy::getInstance()
{
    static DAPyModuleNumpy s_numpy;
    return s_numpy;
}

/**
 * @brief 释放模块资源
 */
void DAPyModuleNumpy::finalize()
{
    d_ptr->del();
}

/**
 * @brief 获取最后的错误信息
 * @return 返回错误信息字符串
 */
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

/**
 * @brief 判断对象是否为numpy.number实例
 * @param obj 要判断的对象
 * @return 如果是number返回true
 */
bool DAPyModuleNumpy::isInstanceNumber(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjNumber);
}

/**
 * @brief 判断对象是否为numpy.integer实例
 * @param obj 要判断的对象
 * @return 如果是integer返回true
 */
bool DAPyModuleNumpy::isInstanceInteger(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjInteger);
}

/**
 * @brief 判断对象是否为numpy.inexact实例
 * @param obj 要判断的对象
 * @return 如果是inexact返回true
 */
bool DAPyModuleNumpy::isInstanceInexact(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjInexact);
}

/**
 * @brief 判断对象是否为numpy.dtype实例
 * @param obj 要判断的对象
 * @return 如果是dtype返回true
 */
bool DAPyModuleNumpy::isInstanceDtype(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjDtype);
}

/**
 * @brief 导入numpy模块
 * @return 导入成功返回true
 */
bool DAPyModuleNumpy::import()
{
    return DAPyModule::import("numpy");
}
}  // namespace DA
