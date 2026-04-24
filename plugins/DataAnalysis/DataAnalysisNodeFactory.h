#ifndef DATAANALYSISNODEFACTORY_H
#define DATAANALYSISNODEFACTORY_H
#include "DataAnalysisGlobal.h"
#include "DAPyNodeFactory.h"
#include <QMap>
#include <QDomDocument>
class QMainWindow;
namespace DA
{
class DACoreInterface;
class DAPyWorkFlowEditWidget;
class DAPyLinkGraphicsItem;
class DAPyWorkFlowGraphicsScene;
class DAPyNodeProxy;
}


class DataAnalysisNodeFactory : public DA::DAPyNodeFactory
{
    Q_OBJECT
public:
    using FpCreate = std::function< DA::DAPyNodeProxy*(void) >;
public:
    DataAnalysisNodeFactory();
    virtual ~DataAnalysisNodeFactory() override;
    // 设置core
    void setCore(DA::DACoreInterface* c);

public:
    /**
     * @brief 工厂的唯一标识
     * @note 每个工厂需要保证有唯一的标识，工作流将通过标识查找工厂
     * @note 此类型名字不能进行翻译
     * @return
     */
    virtual QString factoryPrototypes() const;

    /**
     * @brief 工厂名称
     * @note 工厂名称可进行翻译
     * @return
     */
    virtual QString factoryName() const;

    /**
     * @brief 工厂具体描述
     * @note 工厂具体描述可进行翻译
     * @return
     */
    virtual QString factoryDescribe() const;

    // 节点加入workflow的回调
    virtual void nodeAddedToWorkflow(DA::DAPyNodeProxy* proxy);

    // 节点删除的工厂回调
    virtual void nodeStartRemove(DA::DAPyNodeProxy* proxy);

    // 把扩展信息保存到xml上
    virtual void saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const;
    // 从xml加载扩展信息
    virtual void loadExternInfoFromXml(const QDomElement* factoryExternElement);
public:
    // 窗口相关操作
    //  获取主体窗口
    QMainWindow* getMainWindow() const;

private:
    DA::DACoreInterface* mCore { nullptr };
    QMap< DA::DAPyNodeMetaData, FpCreate > mPrototypeTpfp;
};

#endif  // DATAANALYSISNODEFACTORY_H
