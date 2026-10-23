#ifndef DAAPPCONFIG_H
#define DAAPPCONFIG_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include "DALogger.h"

class QDomElement;

namespace DA
{

/**
 * @brief 日志轮转模式（对应配置 log.rotation-mode）
 */
enum class DALogRotationMode
{
    Rotating = 0,  ///< 按文件大小轮转
    Daily    = 1,  ///< 按日期分割
    Console  = 2   ///< 仅输出到控制台
};

/**
 * @brief 操作窗口内部标签页方位（对应配置 ui.dock-tab-position）
 */
enum class DADockTabPosition
{
    Bottom = 0,  ///< 下方（默认）
    Top    = 1   ///< 上方
};

/**
 * @brief 打开工程时本地工作区与工程内版本的冲突策略（对应配置 script.workspace-overwrite-policy）
 */
enum class DAWorkspaceOverwritePolicy
{
    Ask         = 0,  ///< 每次都询问（默认）
    AlwaysLocal = 1,  ///< 始终保留本地工作区
    AlwaysZip   = 2   ///< 始终使用工程内版本
};

/**
 * @brief 通用/界面设置（对应设置对话框通用页）
 */
struct DAUISettings
{
    int ribbonStyle = 34;  ///< SARibbonBar::RibbonStyleCompactTwoRow（0x02|0x20）
    int ribbonTheme = -1;  ///< SARibbonTheme，-1 表示不覆盖框架默认主题
    QString language;                                              ///< 语言代码，空表示跟随系统
    DADockTabPosition dockTabPosition = DADockTabPosition::Bottom;  ///< 内部标签页方位
    QString fontFamily;                                             ///< 应用字体族，空表示系统默认
    double fontPointSize = -1.0;                                    ///< 应用字号，<=0 表示系统默认
    bool saveUiStateOnClose = false;                                ///< 退出时是否保存界面状态
    int undoStackLimit = 20;                                        ///< 数据页撤销栈深度，0 表示无限制
};

/**
 * @brief 日志设置（对应设置对话框日志页）
 */
struct DALogSettings
{
    DALogLevel level      = DALogLevel::Trace;  ///< 日志文件级别
    DALogLevel queueLevel = DALogLevel::Info;   ///< 界面消息队列级别
    bool outputStdout     = true;               ///< 是否同时输出到 stdout
    DALogRotationMode rotationMode = DALogRotationMode::Rotating;   ///< 轮转模式
    qint64 maxSizeBytes   = qint64(10) * 1024 * 1024;  ///< 单个日志文件最大字节数
    int maxFiles          = 5;                         ///< 保留的历史日志文件数
    int showLogNum        = 5000;                      ///< 界面日志面板显示条数
};

/**
 * @brief Python 环境设置（对应设置对话框 Python 页）
 */
struct DAPythonSettings
{
    QString interpreterPath;  ///< 解释器路径，空表示自动检测
    QStringList extraPaths;   ///< 额外模块搜索路径（追加到 sys.path）
};

/**
 * @brief 高级设置（对应设置对话框高级页）
 */
struct DAAdvancedSettings
{
    int recentFilesMax      = 10;    ///< 最近文件最大条目数
    int dumpRetentionDays   = 7;     ///< 崩溃转储保留天数
    bool showSplash         = true;  ///< 启动时是否显示启动画面
    int autosaveIntervalMin = 0;     ///< 自动保存间隔（分钟），0 表示禁用
};

/**
 * @brief 脚本工作区与脚本执行引擎设置（对应设置对话框高级页的下半部分）
 */
struct DAScriptSettings
{
    QString workspaceDir;  ///< 脚本工作区根目录，空表示使用系统临时目录
    DAWorkspaceOverwritePolicy overwritePolicy = DAWorkspaceOverwritePolicy::Ask;  ///< 工作区覆盖策略
    int scriptTimeoutSec     = 300;    ///< 脚本执行超时（秒），0 表示禁用
    int scriptResultMaxChars = 10000;  ///< 脚本结果序列化后的最大字符数
};

/**
 * @brief 应用配置（强类型配置对象，JSON 存储）
 *
 * 由 main() 创建唯一实例并按构造函数 / setAppConfig / setConfig 注入各消费方，
 * 不使用全局单例。load()/save() 是唯一接触存储格式的代码，字段默认值就地定义，
 * 取值范围校验集中在 normalize()。
 *
 * 任一分节被改写时发出 sectionChanged()，订阅方按分节热应用；changed() 是
 * 不关心分节的便捷信号。加载过程不发信号。
 */
class DAAppConfig : public QObject
{
    Q_OBJECT
public:
    /// 配置分节，用于变更通知的粗粒度路由
    enum class Section
    {
        UI,        ///< 通用/界面
        Log,       ///< 日志
        Python,    ///< Python 环境
        Advanced,  ///< 高级
        Script     ///< 脚本工作区/执行引擎
    };
    Q_ENUM(Section)

    // 构造函数；configFilePath 为空时使用默认配置路径
    explicit DAAppConfig(const QString& configFilePath = QString(), QObject* parent = nullptr);
    ~DAAppConfig() override;

    // 从磁盘加载；文件不存在时按 createIfMissing 决定是否生成默认配置文件
    bool load(bool createIfMissing = true);
    // 保存到磁盘（原子写）
    bool save() const;
    // 恢复全部默认值
    void resetToDefaults();

    const DAUISettings& ui() const;
    void setUI(const DAUISettings& v);
    const DALogSettings& log() const;
    void setLog(const DALogSettings& v);
    const DAPythonSettings& python() const;
    void setPython(const DAPythonSettings& v);
    const DAAdvancedSettings& advanced() const;
    void setAdvanced(const DAAdvancedSettings& v);
    const DAScriptSettings& script() const;
    void setScript(const DAScriptSettings& v);

    // 当前配置文件名
    static QString getConfigFileName();
    // 旧版 XML 配置文件名（仅用于一次性迁移）
    static QString getLegacyConfigFileName();
    // 当前配置文件的绝对路径
    static QString getAbsoluteConfigFilePath();

Q_SIGNALS:
    /// 某一分节被改写
    void sectionChanged(DAAppConfig::Section section);
    /// 任一分节被改写
    void changed();

private:
    void normalize();
    QJsonObject toJson() const;
    void loadFromJsonObject(const QJsonObject& root);
    bool loadLegacyXml(const QString& path);
    void applyLegacyProp(const QString& key, const QDomElement& valueElement);
    QString legacyConfigFilePath() const;

private:
    QString mConfigFilePath;
    DAUISettings mUiSettings;
    DALogSettings mLogSettings;
    DAPythonSettings mPythonSettings;
    DAAdvancedSettings mAdvancedSettings;
    DAScriptSettings mScriptSettings;
    QJsonObject mUnknownRoot;  ///< 加载时的原始根对象，保存时用于保留未知键
};

}  // namespace DA

#endif  // DAAPPCONFIG_H
