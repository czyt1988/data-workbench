#ifndef DAAPPCORE_H
#define DAAPPCORE_H
#include "DACoreInterface.h"

namespace DA
{
class DAAppCommand;
class DAAppUI;
class DAAppRibbonArea;
class DAAppDataManager;
class DAProject;
/**
 * @brief DA的核心接口,作为单例存在
 */
class DAAppCore : public DACoreInterface
{
    Q_OBJECT
    DAAppCore(QObject* p = nullptr);

public:
    static DAAppCore& getInstance();
    //初始化
    virtual bool initialized() override;
    //获取DAAppRibbonAreaInterface
    virtual DAAppUIInterface* getUiInterface() const override;
    //获取项目管理借口
    virtual DAProjectInterface* getProjectInterface() const override;
    //调用此函数，创建DAAppUIInterface，此函数的调用应该发生在SARibbonMainWindow的构造过程
    void createUi(SARibbonMainWindow* mainwindow) override;
    // python内核是否初始化成功
    bool isPythonInterpreterInitialized() const;
    //获取数据管理接口
    DADataManagerInterface* getDataManagerInterface() const override;

public:  // python相关
    //初始化python环境
    bool initializePythonEnv();
    //获取python环境路径
    static QString getPythonInterpreterPath();
    //获取python 脚本路径
    static QString getPythonScriptsPath();

public:
    //获取DAAppUI，省去qobject_cast
    DAAppUI* getAppUi();
    //获取DAProject
    DAProject* getAppProject();
    //获取DAAppDataManager，省去qobject_cast
    DAAppDataManager* getAppDatas();
    //获取DAAppCommand，省去qobject_cast
    DAAppCommand* getAppCmd();

private:
    DAAppCommand* m_appCmd;
    DAAppUI* m_appUI;
    DAAppDataManager* m_dataManager;
    bool m_isPythonInterpreterInitialized;
    DAProject* m_project;
};
}  // namespace DA

#ifndef DA_APP_CORE
/**
 * @def 获取@sa DAAppCore 实例
 */
#define DA_APP_CORE DA::DAAppCore::getInstance()
#endif

#endif  // DACORE_H
