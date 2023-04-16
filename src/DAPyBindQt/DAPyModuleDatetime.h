#ifndef DAPYMODULEDATETIME_H
#define DAPYMODULEDATETIME_H
#include "DAPyBindQtGlobal.h"
#include "DAPyModule.h"
#include <QDateTime>
namespace DA
{
/**
 * @brief python datetime 模块的封装
 */
class DAPYBINDQT_API DAPyModuleDatetime : public DAPyModule
{
    DA_DECLARE_PRIVATE(DAPyModuleDatetime)
    DAPyModuleDatetime();

public:
    ~DAPyModuleDatetime();
    //获取实例
    static DAPyModuleDatetime& getInstance();
    //析构
    void finalize();
    //获取最后的错误
    QString getLastErrorString();
    //
    pybind11::object dateObject();
    pybind11::object dateObject(const QDate& d);
    pybind11::object datetimeObject();
    pybind11::object datetimeObject(const QDateTime& d);
    pybind11::object timeObject();
    pybind11::object timeObject(const QTime& d);
    pybind11::object timedeltaObject();
    //
    bool isInstanceTime(const pybind11::object& obj) const;
    bool isInstanceDate(const pybind11::object& obj) const;
    bool isInstanceDateTime(const pybind11::object& obj) const;
    bool isInstanceTimedelta(const pybind11::object& obj) const;

public:
    //导入模块
    bool import();
};
}  // namespace DA
#endif  // DAPYMODULEDATETIME_H
