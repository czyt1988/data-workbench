#ifndef DATAANALYSISPLUGIN_H
#define DATAANALYSISPLUGIN_H
#include <QtCore/qglobal.h>
#include <QObject>
#include <QAction>
#include "DataAnalysisGlobal.h"
#include "DAAbstractNodePlugin.h"
//
class DataAnalysisUI;
class DataframeIOWorker;
class DataframeCleanerWorker;
class DataframeOperateWorker;
namespace DA
{
class DAPyNodeFactory;
}

class DataAnalysisPlugin : public QObject, public DA::DAAbstractNodePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractNodePlugin)
public:
    DataAnalysisPlugin();
    virtual ~DataAnalysisPlugin() override;
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

    // 创建节点工厂
    virtual DA::DAPyNodeFactory* createNodeFactory() override;

    // 删除节点工厂
    virtual void destroyNodeFactory(DA::DAPyNodeFactory* p) override;
    // 获取设置页
    virtual DA::DAAbstractSettingPage* createSettingPage() override;

    // 翻译
    virtual void retranslate() override;
private Q_SLOTS:
    void onFactoryDestroyed(QObject* obj);

private:
    bool loadSetting();

private:
    DataAnalysisUI* mUi { nullptr };
    DataframeIOWorker* mIoWorker { nullptr };
    DataframeCleanerWorker* mCleanerWorker { nullptr };
    DataframeOperateWorker* mOperateWorker { nullptr };
};

#endif  // DATAANALYSISPLUGIN_H
