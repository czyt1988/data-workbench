#ifndef DAPYINTERPRETER_H
#define DAPYINTERPRETER_H
#include <QtCore/qglobal.h>
#include <QString>
#include "DAPyBindQtGlobal.h"
#include <vector>
#include <functional>
namespace DA
{
/**
 * @brief python 环境管理类
 * 此类设计为单例
 *
 * 这个类的封装是为了可以实现一些结束python环境的回调
 */
class DAPYBINDQT_API DAPyInterpreter
{
    DAPyInterpreter();

public:
    using callback_finalize = std::function< void() >;

public:
    ~DAPyInterpreter();
    // 单例
    static DAPyInterpreter& getInstance();

    /**
     * @brief 执行where python命令，查询所有的python环境
     * @return 如果没有或者异常返回false
     */
    static QList< QString > wherePython();
    // 设置python环境路径
    void setPythonHomePath(const QString& path);
    // 开启python环境
    void initializePythonInterpreter();
    // 注册环境关闭的回调
    void registerFinalizeCallback(callback_finalize fp);

protected:
    // 结束python环境
    void finalizePythonInterpreter();

private:
    std::vector< callback_finalize > mFinalizeCallbacks;
};
}  // namespace DA
#endif  // DAPYINTERPRETER_H
