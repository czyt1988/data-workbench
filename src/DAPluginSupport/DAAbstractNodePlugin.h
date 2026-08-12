#ifndef DAABSTRACTNODEPLUGIN_H
#define DAABSTRACTNODEPLUGIN_H
#include <QtPlugin>
#include "DAPluginSupportGlobal.h"
#include "DAPyNodeFactory.h"
#include "DAAbstractPlugin.h"

namespace DA
{
class DAPyWorkFlowOperateWidget;
/**
 * @brief 节点插件
 *
 *  用户可以继承此节点插件提供节点，具体如下：
 *
 *  工程文件（pro）加入插件的config：
 *
 * @code
 * CONFIG += plugin # 插件
 * @endcode
 *
 *  创建一个插件类，这个插件类必须继承QObject（注意QObject必须是第一个继承），第二个继承为DAAbstractNodePlugin
 *
 *  在头文件加入Q_PLUGIN_METADATA和Q_INTERFACES声明即可实现插件的建立，如下：
 *
 * @code
 * class Q_DECL_EXPORT MyNodePlugin : public QObject, public DAAbstractNodePlugin
 * {
 *  Q_OBJECT
 *  Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
 *  Q_INTERFACES(DA::DAAbstractNodePlugin)
 * };
 * @endcode
 *
 */
class DAPLUGINSUPPORT_API DAAbstractNodePlugin : public DAAbstractPlugin
{
public:
    DAAbstractNodePlugin();
    virtual ~DAAbstractNodePlugin() override;

    // 创建一个节点工厂
    virtual DAPyNodeFactory* createNodeFactory() = 0;
    // 删除一个节点工厂(谁创建谁删除原则)
    virtual void destroyNodeFactory(DAPyNodeFactory* p) = 0;
    // 回调函数，在节点生成完成并加入到APP后调用，此函数默认不做任何动作
    virtual void afterLoadedNodes();
    // 获取当前激活的工作流编辑窗口
    DAPyWorkFlowOperateWidget* getCurrentActiveWorkflowOperateWidget() const;
};
}  // end da
// 封装成插件需要在原本封装dll的基础上添加以下语句
QT_BEGIN_NAMESPACE
#ifndef DAABSTRACTNODEPLUGIN_IID
#define DAABSTRACTNODEPLUGIN_IID "org.da.abstract.nodePlugin"
#endif
Q_DECLARE_INTERFACE(DA::DAAbstractNodePlugin, DAABSTRACTNODEPLUGIN_IID)
QT_END_NAMESPACE

// 继承此插件，需要如下

// Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
// Q_INTERFACES(DA::DAAbstractNodePlugin)
#endif  // DAABSTRACTNODEPLUGIN_H
