#ifndef DAPYINTERPRETER_H
#define DAPYINTERPRETER_H
#include <memory>
#include <QString>
#include <QList>
#include <QFileInfo>
#include "DAPyBindQtGlobal.h"
#include "DAPybind11InQt.h"
namespace DA
{
/**
 * @brief python 环境管理类，这个了封装了pybind11的pybind11::scoped_interpreter
 *
 * 这个类的封装了一些常用方法
 *
 * 1. 设置python环境路径
 * 2. 开启python环境
 * 3. 关闭python环境
 * 4. 获取python配置文件
 * 5. 获取python Interpreter
 * 6. 系统的where命令
 * 7. 从配置文件获取python
 * 8. 获取python环境路径
 *
 * 这个类共享一个全局的pybind11::scoped_interpreter，在首次实例化DAPyInterpreter时会初始化，也可以手动调用DAPyInterpreter::initializePythonInterpreter初始化
 */
class DAPYBINDQT_API DAPyInterpreter
{
public:
    DAPyInterpreter();
    ~DAPyInterpreter();
    // 设置python环境路径
    static void setPythonHomePath(const QString& path);
    // 开启python环境
    static void initializePythonInterpreter();
    // 开启python环境（带Python Home路径参数）
    static void initializePythonInterpreter(const QString& pythonHomePath);
    // 关闭
    static void shutdown();
    static void ensureShutdown();
    // 获取python配置文件
    static QString getAppPythonConfigFile();
    // 获取python Interpreter
    static QString getPythonInterpreterPath();
    // 系统的where命令
    static QList< QFileInfo > wherePython();
    // 从配置文件获取python
    static QList< QFileInfo > wherePythonFromConfig();
    // 是否初始化了python环境
    static bool isPythonInitialized();
    // 添加系统路径
    static void appendSysPath(const QString& path);
public:
    static std::shared_ptr< pybind11::scoped_interpreter > interpreter;
};
}  // namespace DA
#endif  // DAPYINTERPRETER_H
