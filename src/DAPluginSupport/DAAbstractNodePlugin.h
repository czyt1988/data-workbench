#ifndef DAABSTRACTNODEPLUGIN_H
#define DAABSTRACTNODEPLUGIN_H
#include <QtPlugin>
#include "DAPluginSupportGlobal.h"
#include "DAAbstractNodeFactory.h"
#include "DAAbstractPlugin.h"

namespace DA
{
class DAWorkFlowOperateWidget;
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
	virtual ~DAAbstractNodePlugin();

	/**
	 * @brief 创建一个节点工厂
	 * @return
	 */
	virtual DAAbstractNodeFactory* createNodeFactory() = 0;

	/**
	 * @brief 删除一个节点工厂(谁创建谁删除原则)
	 * @param p
	 */
	virtual void destoryNodeFactory(DAAbstractNodeFactory* p) = 0;

	/**
	 * @brief 这是一个回调函数，在节点生成完成，并加入到APP后调用
	 *
	 * 这个函数的作用是等节点都加载到节点管理界面以后进行一些操作，例如对节点进行一些排序操作等，或者在节点树里加入一些其他的item
	 *
	 * 此函数默认不做任何动作
	 */
	virtual void afterLoadedNodes();

    /**
     * @brief 获取当前激活的工作流编辑窗口，所谓当前激活就是当前界面上正在打开的工作流编辑窗口
     * @return
     */
    DAWorkFlowOperateWidget* getCurrentActiveWorkflowOperateWidget() const;

	/**
	 * @brief 获取当前激活的工作流，所谓当前激活就是当前界面上正在打开的工作流
	 * @return
	 */
	DAWorkFlow* getCurrentActiveWorkFlow() const;
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
#endif  // FCABSTRACTNODEPLUGIN_H
