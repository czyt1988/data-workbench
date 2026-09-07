#ifndef DAPLUGINMANAGER_H
#define DAPLUGINMANAGER_H
#include <QObject>
#include "DAPluginSupportGlobal.h"
#include "DAAbstractPlugin.h"
#include "DAPluginOption.h"
#include "DAPluginFileInfo.h"
#include <QDebug>
namespace DA
{

/**
 * @brief 此类为插件管理类，作为单例，管理整个程序的插件加载和释放
 *
 * 支持运行期单个插件的热加载（loadPlugin）和热卸载（unloadPlugin），
 * 启用/禁用状态通过插件目录下的 .pluginignore 文件持久化（setPluginEnabled）
 */
class DAPLUGINSUPPORT_API DAPluginManager : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPluginManager)
public:
    /**
     * @brief 卸载结果
     */
    enum UnloadResult
    {
        UnloadSucceed,   ///< finalize成功且库成功释放
        UnloadDegraded,  ///< finalize成功但库释放失败，插件已注销清理并转入降级停用状态（重启后完全释放）
        UnloadRefused,   ///< 插件finalize拒绝卸载，插件保持加载状态
        UnloadNotFound   ///< 未找到指定插件
    };
    Q_ENUM(UnloadResult)
public:
    explicit DAPluginManager(QObject* p = nullptr);
    virtual ~DAPluginManager() override;
    // 设置忽略列表
    void setIgnoreList(const QStringList& ignorePluginsName);

    // 获取当前忽略（禁用）列表，元素为插件文件基本名（小写）
    QStringList ignoreList() const;

    // 设置插件启用状态并持久化到.pluginignore文件，enable为false表示禁用（下次启动不加载）
    bool setPluginEnabled(const QString& pluginBaseName, bool enable);

    // 加载所有插件
    virtual void loadAllPlugins(DACoreInterface* c);

    // 运行期加载单个插件（热加载），pluginFilePath为插件文件绝对路径
    virtual bool loadPlugin(const QString& pluginFilePath, DACoreInterface* c);

    // 是否已经加载
    bool isLoaded() const;

    // 设置插件路径
    void setPluginPath(const QString& path);

    // 插件数
    int getPluginCount() const;

    // 获取加载的插件名
    QList< QString > getPluginNames() const;

    // 获取所有插件信息
    QList< DAPluginOption > getPluginOptions() const;

    // 扫描插件目录，返回全部插件文件及其状态（含已加载、禁用、加载失败等）
    QList< DAPluginFileInfo > scanPluginFiles() const;

    // 卸载相关
    virtual UnloadResult unloadPlugin(const QString& pluginName);
    virtual bool unloadAllPlugins();


    // 获取插件路径
    static QString getPluginDirPath();

    // 忽略文件名
    static QString getPluginIgnoreFileName();

    // 判断文件后缀是否为当前平台的动态库后缀
    static bool isPluginLibrarySuffix(const QString& suffix);
Q_SIGNALS:
    /**
     * @brief 开始加载插件信号
     *
     * 此信号可以给到启动画面窗口使用
     * @param pluginPath
     */
    void beginLoadPlugin(const QString& pluginPath);

    /**
     * @brief 单个插件加载成功（热加载）
     * @param pluginFilePath 插件文件绝对路径
     */
    void pluginLoaded(const QString& pluginFilePath);

    /**
     * @brief 插件卸载
     * @param pluginName 插件名
     * @param fullyUnloaded true表示库已成功释放，false表示降级停用（库驻留内存，重启后释放）
     */
    void pluginUnloaded(const QString& pluginName, bool fullyUnloaded);
};

// 格式化输出
DAPLUGINSUPPORT_API QDebug operator<<(QDebug debug, const DAPluginManager& fmg);
}  // namespace DA

#endif  // DAPLUGINMANAGER_H
