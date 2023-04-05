#ifndef DAUTILNODEFACTORY_H
#define DAUTILNODEFACTORY_H
#include "DAUtilityNodePluginAPI.h"
#include "DAAbstractNodeFactory.h"
#include <functional>
namespace DA
{
class DAAbstractNodeWidget;
class DACoreInterface;
/**
 * @brief 主节点工厂
 *
 * 其他的插件可参看主节点工厂进行编写
 */
class DAUTILITYNODEPLUGIN_API DAUtilityNodeFactory : public DAAbstractNodeFactory
{
    Q_OBJECT
public:
    using FpCreate = std::function< DAAbstractNode::SharedPointer(void) >;

public:
    DAUtilityNodeFactory(DACoreInterface* c, QObject* p = nullptr);
    //工程类型
    virtual QString factoryPrototypes() const override;
    //工厂名称
    QString factoryName() const override;
    //工厂描述
    virtual QString factoryDescribe() const override;
    //工厂函数，创建一个FCNodeGraphicsItem，工厂不持有FCNodeGraphicsItem的管理权
    DAAbstractNode::SharedPointer create(const DANodeMetaData& meta) override;

    //获取所有注册的Prototypes
    QStringList getPrototypes() const override;

    //获取所有类型的分组，同样的分组会放置在一起，这样一个工厂不仅仅只对应一个分组
    QList< DANodeMetaData > getNodesMetaData() const override;

    //通过工厂创建node setting widget,避免每个node都创建一个widget，导致widget过多
    DAAbstractNodeWidget* createNodeSettingWidget(const DAAbstractNode::SharedPointer& p);
    //获取配置映射
    static QHash< QString, QPair< QString, QStringList > > getSettingMap();

    //获取core
    DACoreInterface* core() const;

public:
    static QHash< QString, QPair< QString, QStringList > > s_settingMap;

protected:
    void createMetaData();

private:
    QMap< DANodeMetaData, FpCreate > m_prototypeTpfp;
    DACoreInterface* _core;
};
}

#endif  // FCNODEGRAPHICSFACTORY_H
