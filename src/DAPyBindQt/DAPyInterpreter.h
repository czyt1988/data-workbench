#ifndef DAPYINTERPRETER_H
#define DAPYINTERPRETER_H
#include <QtCore/qglobal.h>
#include <QString>
#include "DAPyBindQtGlobal.h"
#include <vector>
#include <functional>
#include <QFileInfo>
namespace DA
{
/**
 * @brief python 环境管理类，这个了封装了pybind11的pybind11::scoped_interpreter
 *
 * 这个类的封装是为了可以实现一些结束python环境的回调
 */
class DAPYBINDQT_API DAPyInterpreter
{
    DA_DECLARE_PRIVATE(DAPyInterpreter)
public:
    DAPyInterpreter();
    ~DAPyInterpreter();
    // 设置python环境路径
    void setPythonHomePath(const QString& path);
    // 开启python环境
    void initializePythonInterpreter();
    // 关闭
    void shutdown();
    void ensureShutdown();
    // 获取python配置文件
    static QString getAppPythonConfigFile();
    // 获取python Interpreter
    static QString getPythonInterpreterPath();
    // 系统的where命令
    static QList< QFileInfo > wherePython();
    // 从配置文件获取python
    static QList< QFileInfo > wherePythonFromConfig();
};
}  // namespace DA
#endif  // DAPYINTERPRETER_H
