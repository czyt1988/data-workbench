#ifndef DASYSTEMNODESPLUGIN_H
#define DASYSTEMNODESPLUGIN_H
#include <QtCore/qglobal.h>
#include <QObject>
#include "DASystemNodesGlobal.h"
#include "DAAbstractNodePlugin.h"

namespace DA
{
class DAPyNodeFactory;
}

class DASYSTEMNODES_API DASystemNodesPlugin : public QObject, public DA::DAAbstractNodePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractNodePlugin)
public:
    DASystemNodesPlugin();
    virtual ~DASystemNodesPlugin() override;

    // 初始化
    virtual bool initialize() override;

    // 插件id
    virtual QString getIID() const override;

    // 插件名
    virtual QString getName() const override;

    // 插件版本
    virtual QString getVersion() const override;

    // 插件描述
    virtual QString getDescription() const override;

    // 创建一个节点工厂，纯 Python 插件返回 nullptr
    virtual DA::DAPyNodeFactory* createNodeFactory() override;

    // 删除一个节点工厂
    virtual void destroyNodeFactory(DA::DAPyNodeFactory* p) override;

    // 获取设置页
    virtual DA::DAAbstractSettingPage* createSettingPage() override;

    // 翻译
    virtual void retranslate() override;
};

#endif  // DASYSTEMNODESPLUGIN_H
