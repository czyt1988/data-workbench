#ifndef DAABSTRACTPLUGIN_H
#define DAABSTRACTPLUGIN_H
#include <QtPlugin>
#include <QObject>
#include "DAPluginSupportGlobal.h"
namespace DA
{
class DACoreInterface;
DA_IMPL_FORWARD_DECL(DAAbstractPlugin)

/**
 * @brief 所有插件的基类
 *
 * 所有支持插件的库都需要实现以下导出函数
 * DAAbstractPlugin* plugin_create();
 * void plugin_destory(DAAbstractPlugin* p);
 */
class DAPLUGINSUPPORT_API DAAbstractPlugin
{
    DA_IMPL(DAAbstractPlugin)
    friend class DAPluginOption;
    friend class DAPluginManager;

public:
    DAAbstractPlugin();
    virtual ~DAAbstractPlugin();

    //插件id
    virtual QString getIID() const = 0;

    /**
     * @brief 插件名
     * @return
     */
    virtual QString getName() const = 0;

    /**
     * @brief 插件版本
     * @return
     */
    virtual QString getVersion() const = 0;

    /**
     * @brief 插件描述
     * @return
     */
    virtual QString getDescription() const = 0;

    /**
     * @brief 发生语言变更事件的时候调用此函数
     * 默认没有实现，如果插件有涉及翻译，需要重载此函数
     */
    virtual void retranslate();

    /**
     * @brief 初始化
     * @return 如果初始化返回false，将不会把插件放入管理中，默认返回true
     */
    virtual bool initialize();

    //获取core
    DACoreInterface* core() const;

protected:
    void setCore(DACoreInterface* c);
};
}  // namespace DA

//封装成插件需要在原本封装dll的基础上添加以下语句
QT_BEGIN_NAMESPACE
#define DAABSTRACTPLUGIN_IID "org.da.abstract.plugin"
Q_DECLARE_INTERFACE(DA::DAAbstractPlugin, DAABSTRACTPLUGIN_IID)
QT_END_NAMESPACE

#endif  // FCABSTRACTPLUGIN_H
