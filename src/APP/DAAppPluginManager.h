#ifndef DAAPPPLUGINMANAGER_H
#define DAAPPPLUGINMANAGER_H

#include <QtCore/qglobal.h>
#include "DAGlobals.h"
#include "DACoreInterface.h"
#include <QObject>
#include <QList>
#include <QSet>
#include <memory>

#include "DAPyNodeFactory.h"
#include "DAPluginManager.h"
namespace DA
{
class _DAPrivateWorkflowNodePluginData;
class DAAbstractNodePlugin;
class DAAbstractPlugin;
class DAPyNodeFactory;

/**
 * @brief Python节点包信息（pyplugins目录下的Python包）
 *
 * Python节点包无法在运行期安全卸载，禁用后下次启动生效
 */
struct DAPyPluginPackageInfo
{
    QString packageName;     ///< 包目录名
    QString dirPath;         ///< 包目录绝对路径
    bool enabled { true };   ///< 是否启用（禁用项记录在pyplugins/.pluginignore，下次启动不加载）
};

/**
 * @brief 此app的插件管理类
 *
 * 在DAPluginManager基础上增加：
 * 1. C++插件的运行期热插拔编排（enablePlugin/disablePlugin），
 *    卸载前完成宿主侧注册表注销（agent工具/系统提示词）与在用节点守卫
 * 2. Python节点包（pyplugins目录）的启用状态管理（重启生效）
 */
class DAAppPluginManager : public DAPluginManager
{
    Q_OBJECT
public:
    DAAppPluginManager(QObject* p = nullptr);
    ~DAAppPluginManager() override;

    // 初始化加载所有插件
    virtual void loadAllPlugins(DACoreInterface* c) override;

    // 获取所有的插件
    QList< DAAbstractPlugin* > getAllPlugins() const;

    // 获取所有的节点插件
    QList< DAAbstractNodePlugin* > getNodePlugins() const;

    // 获取所有的节点工厂
    QList< std::shared_ptr< DAPyNodeFactory > > createNodeFactorys() const;

    // 获取所有的元数据
    QList< DAPyNodeMetaData > getAllNodeMetaDatas() const;

    // 获取Python节点工厂（可能为nullptr如果Python未启用或初始化失败）
    std::shared_ptr< DAPyNodeFactory > getPyNodeFactory() const;

    // 运行期启用C++插件（热加载）并持久化启用状态，baseName为插件文件基本名
    bool enablePlugin(const QString& pluginBaseName, QString* errorString = nullptr);

    // 运行期禁用C++插件（热卸载）并持久化禁用状态，baseName为插件文件基本名；
    // 插件节点正被打开的工作流使用时拒绝禁用；库释放失败时转入降级停用（重启后完全释放），仍返回true
    bool disablePlugin(const QString& pluginBaseName, QString* errorString = nullptr);

    // 获取pyplugins目录下全部Python节点包信息（含禁用项）
    QList< DAPyPluginPackageInfo > getPyPluginPackageInfos() const;

    // 设置Python节点包启用状态，持久化到pyplugins/.pluginignore，下次启动生效
    bool setPyPluginEnabled(const QString& packageName, bool enable);

    // 获取pyplugins目录绝对路径
    static QString getPyPluginDirPath();

Q_SIGNALS:
    // 节点元数据变化（插件热插拔后），UI据此刷新节点工具箱
    void nodeMetaDatasChanged();

private:
    // 初始化Python节点工厂
    void initPyNodeFactory();
    // 按当前已加载插件重建mPlugins/mNodeMetaDatas（含Python工厂元数据合并与去重）
    void refreshAfterPluginChange();
    // 检查节点插件的节点是否被打开的工作流使用，返回在用的节点qualifiedName列表
    QStringList pluginNodesInUse(DAAbstractNodePlugin* np) const;
    // 按文件基本名查找已加载的插件选项下标，未找到返回-1
    int findPluginOptionIndexByBaseName(const QString& pluginBaseName) const;
    // 读取pyplugins的忽略列表（小写包名集合）
    static QSet< QString > readPyPluginIgnoreSet();
    // 将包名集合写入pyplugins/.pluginignore
    static bool writePyPluginIgnoreSet(const QSet< QString >& ignoreSet);

    QList< DAAbstractPlugin* > mPlugins;
    QList< DAPyNodeMetaData > mNodeMetaDatas;
    std::shared_ptr< DAPyNodeFactory > mPyNodeFactory;  // Python节点工厂
    DACoreInterface* mCore { nullptr };                 // 核心接口，loadAllPlugins时记录
};
}  // namespace DA
#endif  // FCMETHODEDITORPLUGINMANAGER_H
