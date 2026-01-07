#ifndef DAPYMODULE_H
#define DAPYMODULE_H
#include "DAPyBindQtGlobal.h"
#include "DAPybind11InQt.h"
#include "DAPyObjectWrapper.h"
namespace DA
{

/**
 * @brief 模块的基类
 */
class DAPYBINDQT_API DAPyModule : public DAPyObjectWrapper
{
public:
    DAPyModule();
    DAPyModule(const char* moduleName);
    DAPyModule(const pybind11::object& obj);
    DAPyModule(pybind11::object&& obj);
    virtual ~DAPyModule();
    DAPyModule& operator=(const DAPyObjectWrapper& obj);
    DAPyModule& operator=(const pybind11::object& obj);

public:
    // 判断模块是否导入
    bool isImport() const;
    // 获取module的名字
    QString moduleName() const;
    // 重新加载模块
    void reload();
    // 导入模块
    bool import(const char* module_n) noexcept;
    // 异常信息
    QString lastError() const;
    bool hasError() const;
    void clearError();

public:
    static DAPyModule importModule(const char* module_n);

public:
    // 常用类型的判断
    static bool isInstanceTime(const pybind11::handle& obj);
    static bool isInstanceDate(const pybind11::handle& obj);
    static bool isInstanceDateTime(const pybind11::handle& obj);
    static bool isInstanceTimedelta(const pybind11::handle& obj);
};
}  // namespace DA
#endif  // DAPYMODULE_H
