#ifndef DAABSTRACTPLUGIN_H
#define DAABSTRACTPLUGIN_H
#include <memory>
#include <QtPlugin>
#include <QObject>
#include "DAPluginSupportGlobal.h"
#include "DAAbstractSettingPage.h"
namespace DA
{
class DACoreInterface;
class DAAbstractArchiveTask;
/**
 * @brief 所有插件的基类
 *
 * 所有支持插件的库都需要实现以下导出函数
 * DAAbstractPlugin* plugin_create();
 * void plugin_destory(DAAbstractPlugin* p);
 */
class DAPLUGINSUPPORT_API DAAbstractPlugin
{
    DA_DECLARE_PRIVATE(DAAbstractPlugin)
    friend class DAPluginOption;
    friend class DAPluginManager;

public:
    DAAbstractPlugin();
    virtual ~DAAbstractPlugin();

    // 插件id
    virtual QString getIID() const = 0;
    // 插件名
    virtual QString getName() const = 0;
    // 插件版本
    virtual QString getVersion() const = 0;
    // 插件描述
    virtual QString getDescription() const = 0;
    // 发生语言变更事件的时候调用此函数，默认没有实现，如果插件有涉及翻译，需要重载此函数
    virtual void retranslate();
    // 初始化，所有针对界面的操作都应该在initialize里调用，默认返回true，返回false将不会把插件放入管理中
    virtual bool initialize();
    // 释放插件的回调函数，默认返回true，返回false说明插件卸载失败
    virtual bool finalize();
    // 获取设置页，默认返回nullptr，代表没有设置页
    virtual DAAbstractSettingPage* createSettingPage();
    // 创建存档任务，isSave为true表示保存任务，默认返回nullptr
    virtual std::shared_ptr< DAAbstractArchiveTask > createArchiveTask(bool isSave);

    // 获取core
    DACoreInterface* core() const;

protected:
    void setCore(DACoreInterface* c);
};
}  // namespace DA

// 封装成插件需要在原本封装dll的基础上添加以下语句
QT_BEGIN_NAMESPACE
#define DAABSTRACTPLUGIN_IID "org.da.abstract.plugin"
Q_DECLARE_INTERFACE(DA::DAAbstractPlugin, DAABSTRACTPLUGIN_IID)
QT_END_NAMESPACE

#endif  // DAABSTRACTPLUGIN_H
