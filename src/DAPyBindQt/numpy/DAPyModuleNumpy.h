#ifndef DAPYMODULENUMPY_H
#define DAPYMODULENUMPY_H
#include "DAPyBindQtGlobal.h"
#include "DAPyModule.h"
namespace DA
{
DA_IMPL_FORWARD_DECL(DAPyModuleNumpy)
/**
 * @brief numpy
 */
class DAPYBINDQT_API DAPyModuleNumpy : public DAPyModule
{
    DA_IMPL(DAPyModuleNumpy)
    DAPyModuleNumpy();

public:
    ~DAPyModuleNumpy();
    //获取实例
    static DAPyModuleNumpy& getInstance();
    //析构
    void finalize();
    //获取最后的错误
    QString getLastErrorString();
    //判断是否为numpy.generic
    bool isInstanceGeneric(const pybind11::object& obj) const;
    bool isInstanceNumber(const pybind11::object& obj) const;
    bool isInstanceInteger(const pybind11::object& obj) const;
    bool isInstanceInexact(const pybind11::object& obj) const;
    bool isInstanceDtype(const pybind11::object& obj) const;

public:
    //导入模块
    bool import();
};
}  // namespace DA
#endif  // DANUMPY_H
