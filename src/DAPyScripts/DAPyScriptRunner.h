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
 * @brief 持久命名空间的脚本执行引擎（类 Jupyter kernel 模型）
 *
 * 命名空间按"当前执行会话"分桶（决策点 3 方案 c，审计问题 13）：
 * - 默认命名空间：无会话上下文时使用（工作流节点等非 agent 调用方，
 *   行为与历史完全一致）；
 * - 会话命名空间：DAPyScriptSessionContext 守卫设置会话 id 后，
 *   runCode/runScript 在该会话专属变量表执行并累积变量——同一 agent
 *   会话内 Jupyter 式体验不变（变量跨调用持久），并发会话之间互不可见
 *   （会话 B 的 df 不再静默改写会话 A 正在使用的 df）。
 * 各表均含预导入的 {da_app, da_interface, da_data} 基线模块；跨会话
 * 故意共享数据走 da_app.getCore().getDataManagerInterface().
 * getAllDataframes() 全局数据入口（内存对象，不经文件）。
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
 * @note 会话上下文（DAPyScriptSessionContext/currentSessionId）仅主线程
 * 使用——工具执行按决策点 2 保持主线程串行，无跨线程竞争
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
    // 清理用户符号回基线（工程清空/切换时调用；作用于当前会话上下文的表）
    static void resetNamespace();
    // 获取命名空间 dict（唯一例外：要求调用方自行持有 GIL；当前会话上下文
    // 的表，无上下文/未创建时回退默认表）
    static pybind11::dict getNamespace();
    // 移除并释放指定会话的命名空间（会话删除时由 Module 调用；析构 dict
    // 内部持 GIL）。会话存续期间桥退役不清——变量随会话生命周期（决策点 3：
    // 同一会话内跨 run_code 持久，切走切回/桥重建对用户无感）
    static void removeSessionNamespace(const QString& sessionId);
    // 当前会话上下文 id（DAPyScriptSessionContext 设置；空=默认命名空间）
    static QString currentSessionId();
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
    // 取（懒建）会话专属命名空间——调用前必须已持 GIL；基线导入失败抛
    // pybind11::error_already_set 由调用方 runInternal 的 catch 兜底
    static pybind11::dict sessionNamespaceLocked(const QString& sessionId);

private:
    class InnerData;
    static std::unique_ptr< InnerData > s_data;
};

/**
 * @brief "当前执行会话"RAII 上下文守卫（决策点 3 方案 c，审计问题 13）
 *
 * DAAgentBridge::executeToolNow 入口以桥所属会话 id 构造，作用域内
 * runCode/runScript 选用该会话的专属变量表；析构恢复上一层上下文
 *（嵌套安全）。不改变 DAAbstractAgentTool::execute 公开 API——工具
 * 实现无需感知会话身份。仅主线程使用（工具执行全局串行）。
 */
class DAPYSCRIPTS_API DAPyScriptSessionContext
{
public:
    // 进入会话上下文（sessionId 空=默认命名空间）
    explicit DAPyScriptSessionContext(const QString& sessionId);
    // 恢复上一层上下文
    ~DAPyScriptSessionContext();
    DAPyScriptSessionContext(const DAPyScriptSessionContext&)            = delete;
    DAPyScriptSessionContext& operator=(const DAPyScriptSessionContext&) = delete;

private:
    QString mPrevious;  ///< 上一层上下文 id（析构恢复）
};
}  // namespace DA
#endif  // DAPYSCRIPTRUNNER_H
