#ifndef DAUTILITYNODEPLUGIN_H
#define DAUTILITYNODEPLUGIN_H
#include <QtCore/qglobal.h>
#include <QObject>
#include <QHash>
#include <memory>
#include "DANodeTreeWidget.h"
#include "DAAbstractNodePlugin.h"
#include "DAUtilityNodePluginAPI.h"

class QTreeWidgetItem;
class WorkflowTemplateDialog;
class SARibbonCategory;
class SARibbonPannel;
class SARibbonBar;

namespace DA
{

/**
 * @brief 基础插件
 */
class DAUTILITYNODEPLUGIN_API DAUtilityNodePlugin : public QObject, public DAAbstractNodePlugin
{
    Q_OBJECT
    //插件支持，所有的插件都需要写入此信息
    Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractNodePlugin)
public:
    DAUtilityNodePlugin(QObject* par = nullptr);
    ~DAUtilityNodePlugin();
    virtual bool initialize();
    //插件id
    virtual QString getIID() const;

    /**
     * @brief 插件名
     * @return
     */
    virtual QString getName() const;

    /**
     * @brief 插件版本
     * @return
     */
    virtual QString getVersion() const;

    /**
     * @brief 插件描述
     * @return
     */
    virtual QString getDescription() const;

    /**
     * @brief 创建一个节点工厂
     * @return
     */
    virtual DAAbstractNodeFactory* createNodeFactory();

    /**
     * @brief 发生语言变更
     */
    virtual void retranslate();

    /**
     * @brief 删除一个节点工厂(谁创建谁删除原则)
     * @param p
     */
    virtual void destoryNodeFactory(DAAbstractNodeFactory* p);

private:
    void init();
};
}  // DA

#endif  // FCUTILNODEPLUGIN_H
