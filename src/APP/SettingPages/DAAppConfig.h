#ifndef DACOMMONCONFIG_H
#define DACOMMONCONFIG_H
#include "DAProperties.h"
#include "DAXMLFileInterface.h"
#include "DAAppCore.h"
#include <QVersionNumber>
/**
 *@def ribbon的样式
 */
#define DA_CONFIG_KEY_RIBBON_STYLE "ribbon-style"
/**
 *@def 显示日志的条数
 */
#define DA_CONFIG_KEY_SHOW_LOG_NUM "show-log-num"
/**
 *@def 程序在退出时是否保存ui的状态
 */
#define DA_CONFIG_KEY_SAVE_UI_STATE_ON_CLOSE "save-ui-state-on-close"
/**
 *@def 语言代码，如 "zh_CN"/"en_US"，空字符串表示跟随系统
 */
#define DA_CONFIG_KEY_LANGUAGE "language"
/**
 *@def ribbon 主题色，对应 SARibbonTheme 枚举
 */
#define DA_CONFIG_KEY_RIBBON_THEME "ribbon-theme"
/**
 *@def 应用字体族名，空字符串表示使用系统默认
 */
#define DA_CONFIG_KEY_APP_FONT_FAMILY "app-font-family"
/**
 *@def 应用字体字号，<=0 表示使用系统默认
 */
#define DA_CONFIG_KEY_APP_FONT_POINT_SIZE "app-font-size"
/**
 *@def Python 解释器路径，空表示自动检测
 */
#define DA_CONFIG_KEY_PYTHON_INTERPRETER_PATH "python-interpreter-path"
/**
 *@def Python 额外模块搜索路径(sys.path)
 */
#define DA_CONFIG_KEY_PYTHON_EXTRA_PATHS "python-extra-paths"
/**
 *@def 日志级别，对应 DALogLevel 枚举
 */
#define DA_CONFIG_KEY_LOG_LEVEL "log-level"
/**
 *@def UI 日志队列级别，对应 DALogLevel 枚举
 */
#define DA_CONFIG_KEY_LOG_QUEUE_LEVEL "log-queue-level"
/**
 *@def 是否输出日志到 stdout
 */
#define DA_CONFIG_KEY_LOG_OUTPUT_STDOUT "log-output-stdout"
/**
 *@def 日志轮转模式：0=rotating,1=daily,2=console
 */
#define DA_CONFIG_KEY_LOG_ROTATION_MODE "log-rotation-mode"
/**
 *@def 单个日志文件最大字节数
 */
#define DA_CONFIG_KEY_LOG_MAX_SIZE "log-max-size"
/**
 *@def 保留的历史日志文件数
 */
#define DA_CONFIG_KEY_LOG_MAX_FILES "log-max-files"
/**
 *@def 工作流执行超时秒数，<0 表示无限等待
 */
#define DA_CONFIG_KEY_WORKFLOW_TIMEOUT "workflow-timeout"
/**
 *@def 最近打开文件最大条目数
 */
#define DA_CONFIG_KEY_RECENT_FILES_MAX "recent-files-max"
/**
 *@def 崩溃转储保留天数
 */
#define DA_CONFIG_KEY_DUMP_RETENTION_DAYS "dump-retention-days"
/**
 *@def 节点脚本额外搜索路径
 */
#define DA_CONFIG_KEY_NODE_SCRIPT_PATHS "node-script-paths"
/**
 *@def 插件额外搜索路径
 */
#define DA_CONFIG_KEY_PLUGIN_EXTRA_PATHS "plugin-extra-paths"
/**
 *@def 是否显示启动画面
 */
#define DA_CONFIG_KEY_SHOW_SPLASH "show-splash"
/**
 *@def 自动保存间隔(分钟)，0 表示禁用
 */
#define DA_CONFIG_KEY_AUTOSAVE_INTERVAL "autosave-interval"

namespace DA
{
class AppMainWindow;
/**
 * @brief 此类为本程序的设置类
 */
class DAAppConfig : public DAProperties, public DAXMLFileInterface
{
public:
    DAAppConfig();
    virtual ~DAAppConfig();
    void setCore(DAAppCore* core);
    //加载配置
    bool loadConfig(bool noFileCreateNewOne = true);
    //保存配置
    bool saveConfig();
    //保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement,const QVersionNumber& ver) const override;
    virtual bool loadFromXml(const QDomElement* parentElement,const QVersionNumber& ver) override;
    //获取配置文件名字
    static QString getConfigFileName();
    //获取配置文件的绝对路径
    static QString getAbsoluteConfigFilePath();

public:
    virtual bool apply();

private:
private:
    const QString cConfigName { "da-app" };
    QString mConfigFilePath;  ///< 配置文件路径
    DAAppCore* mCore { nullptr };
    DAAppUI* mUI { nullptr };
    AppMainWindow* mMainWindow { nullptr };
    QVersionNumber mVersion;
};
}

#endif  // DACOMMONCONFIG_H
