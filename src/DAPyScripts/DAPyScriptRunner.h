#ifndef DAPYSCRIPTRUNNER_H
#define DAPYSCRIPTRUNNER_H
#include <QJsonObject>
#include <QString>
#include <memory>
#include "DAPyScriptsGlobal.h"
#include "DAPybind11InQt.h"

namespace DA
{
/**
 * @brief 共享持久命名空间的脚本执行引擎（类 Jupyter kernel 模型）
 *
 * 整个应用全局唯一命名空间，脚本在其中执行并累积变量（含预导入的
 * {da_app, da_interface, da_data} 基线模块）。数据访问走内存对象
 * （da_app.getCore().getDataManagerInterface().getAllDataframes()），
 * 不经过文件，因此不存在工程数据同步问题。
 *
 * 生命周期由 DACoreInterface 管理：initializePythonScripts() 中
 * DAPyScripts::initScripts() 成功后调用 init()，~DACoreInterface() 中
 * DAPyScripts::cleanup() 之前调用 cleanup()。内部持有
 * scoped_interpreter 的 shared_ptr 以抬高解释器引用计数，保证命名空间
 * dict 不会在解释器销毁后才析构。
 *
 * @note 脚本在主线程同步执行，会阻塞 Qt 事件循环（UI 冻结），
 * 由可配置超时（默认 300 秒，0=禁用）注入 KeyboardInterrupt 兜底
 * @note GIL 契约：除 getNamespace() 外，所有触碰 Python 的方法内部自持
 * DAPyGILGuard，调用方无需（也不应）预先持有 GIL
 */
class DAPYSCRIPTS_API DAPyScriptRunner
{
public:
    // 初始化命名空间，预导入基线模块 {da_app, da_interface, da_data}
    static bool init();
    // 判断命名空间引擎是否已初始化
    static bool isInit();
    // 清理（必须在 Python 解释器关闭前调用）
    static void cleanup();
    // 设置脚本工作区根目录，管理 sys.path 的追加/移除；传空串表示清除
    static void setWorkspaceRoot(const QString& dir);
    // 按绝对路径执行 .py 文件，absPath 由调用方解析并做边界校验
    static QJsonObject runScript(const QString& absPath, const QJsonObject& args = QJsonObject());
    // 执行内联代码字符串（agent run_code 用），在同一持久命名空间执行
    static QJsonObject runCode(const QString& code, const QJsonObject& args = QJsonObject());
    // 清理用户符号回基线（工程清空/切换时调用）
    static void resetNamespace();
    // 获取命名空间 dict（唯一例外：要求调用方自行持有 GIL）
    static pybind11::dict getNamespace();
    // 设置脚本执行超时（秒），0=禁用；无需 GIL
    static void setScriptTimeout(int seconds);
    // 获取脚本执行超时（秒）
    static int getScriptTimeout();
    // 设置结果序列化后的最大字符数，超长截断；无需 GIL
    static void setResultMaxChars(int maxChars);
    // 获取结果最大字符数
    static int getResultMaxChars();

private:
    // runScript/runCode 的公共执行流程（前置校验后的主体）
    static QJsonObject runInternal(bool isFile, const QString& codeOrPath, const QJsonObject& args);

private:
    class InnerData;
    static std::unique_ptr< InnerData > s_data;
};
}  // namespace DA
#endif  // DAPYSCRIPTRUNNER_H
