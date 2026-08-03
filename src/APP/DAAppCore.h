#ifndef DAAPPCORE_H
#define DAAPPCORE_H
#include "DACoreInterface.h"

namespace DA
{
class DAAppCommand;
class DAAppUI;
class DAAppRibbonArea;
class DAAppDataManager;
class DAAppProject;
class DAAgentInterface;
/**
 * @brief DA的核心接口,作为单例存在
 */
class DAAppCore : public DACoreInterface
{
    Q_OBJECT
    DAAppCore(QObject* p = nullptr);

public:
    static DAAppCore& getInstance();
    // 初始化
    virtual bool initialized() override;
    // 获取DAAppRibbonAreaInterface
    virtual DAUIInterface* getUiInterface() const override;
    // 获取项目管理借口
    virtual DAProjectInterface* getProjectInterface() const override;
    // 调用此函数，创建DAAppUIInterface，此函数的调用应该发生在SARibbonMainWindow的构造过程
    void createUi(SARibbonMainWindow* mainwindow) override;
    // 获取数据管理接口
    DADataManagerInterface* getDataManagerInterface() const override;
    // 获取Agent接口（本期返回nullptr，真实实现由后续plan提供）
    virtual DAAgentInterface* getAgentInterface() const override;

public:
    // 获取DAAppUI，省去qobject_cast
    DAAppUI* getAppUi();
    // 获取DAProject
    DAAppProject* getAppProject();
    // 获取DAAppDataManager，省去qobject_cast
    DAAppDataManager* getAppDatas();
    // 获取DAAppCommand，省去qobject_cast
    DAAppCommand* getAppCmd();

private:
    DAAppCommand* mAppCmd { nullptr };
    DAAppUI* mAppUI { nullptr };
    DAAppDataManager* mDataManager { nullptr };
    bool mIsPythonInterpreterInitialized { false };
    DAAppProject* mProject { nullptr };
    DAAgentInterface* mAgentInterface { nullptr };  ///< Agent 接口（initialized 时创建）
};

DACoreInterface* getAppCorePtr();
}  // namespace DA

#ifndef DA_APP_CORE
/**
 * @def 获取@sa DAAppCore 实例
 */
#define DA_APP_CORE DA::DAAppCore::getInstance()
#endif

#endif  // DACORE_H
