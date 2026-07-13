#include "DAPyScripts.h"
#include <QObject>
#include <QDebug>
#include "DAPyScriptsIO.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyInterpreter.h"
#include "DALogCategory.h"

namespace DA
{

class DAPyScripts::InnerModules
{
public:
    InnerModules()
    {
        // 初始化时，把python环境的interpreter赋值给interpreter，让它持有python环境的引用计数，避免此类晚于python环境析构，导致python环境析构了，此类还存在
        interpreter = DAPyInterpreter::interpreter;
        if (!interpreter) {
            // Python 环境未初始化时抛出异常，由 initScripts() 的 try-catch 捕获并返回 false，
            // 避免在 Python 环境未就绪时继续构造子模块导致级联异常
            throw std::runtime_error("DAPyInterpreter is not initialized before DAPyScripts::InnerModules construction");
        }
    }

public:
    DAPyWorkBench workBench;
    std::shared_ptr< pybind11::scoped_interpreter > interpreter;  ///< 解析器，为了增加引用计数，避免python环境析构了，此类还存在
};

std::unique_ptr< DAPyScripts::InnerModules > DAPyScripts::s_models = nullptr;
//===================================================
// DAPyScripts
//===================================================
DAPyScripts::DAPyScripts()
{
    initScripts();  // 初始化脚本,如果已经初始化，不会在执行
}

DAPyScripts::~DAPyScripts()
{
}

/**
 * @brief 此函数在main函数中调用，若失败应用程序考虑是否继续
 * @param err
 * @return
 */
bool DAPyScripts::isInitScripts()
{
    return s_models != nullptr;
}

/**
 * @brief 初始化脚本
 *
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool DAPyScripts::initScripts()
{
    try {
        if (DAPyInterpreter::isPythonInitialized() && !s_models) {
            // 确保只会一次调用，且是首次构造时调用，避免重复加载
            s_models = std::make_unique< InnerModules >();
            return true;
        }
    } catch (const std::exception& e) {
        daCritical << QObject::tr("Failed to initialize import scripts: %1").arg(e.what());  // cn:初始化导入脚本失败：%1
    }
    return false;
}

/**
 * @brief 获取io相关的script
 * @return
 * @note 调用前必须确保 isInitScripts() 返回 true
 */
DAPyScriptsIO& DAPyScripts::getIO()
{
    if (!s_models) {
        // 理论上不应走到这里，调用方应在 isInitScripts() 为 true 后再调用
        daCritical << QObject::tr("DAPyScripts is not initialized, getIO() called before initScripts()");  // cn:DAPyScripts 未初始化，在 initScripts() 之前调用了 getIO()
        Q_ASSERT_X(s_models, "DAPyScripts::getIO", "s_models is nullptr");
    }
    return s_models->workBench.getIO();
}

/**
 * @brief 获取dataframe相关的script
 * @return
 * @note 调用前必须确保 isInitScripts() 返回 true
 */
DAPyScriptsDataFrame& DAPyScripts::getDataFrame()
{
    if (!s_models) {
        // 理论上不应走到这里，调用方应在 isInitScripts() 为 true 后再调用
        daCritical << QObject::tr("DAPyScripts is not initialized, getDataFrame() called before initScripts()");  // cn:DAPyScripts 未初始化，在 initScripts() 之前调用了 getDataFrame()
        Q_ASSERT_X(s_models, "DAPyScripts::getDataFrame", "s_models is nullptr");
    }
    return s_models->workBench.getDataFrame();
}

DAPyScriptsDataProcess& DAPyScripts::getDataProcess()
{
    if (!s_models) {
        // 理论上不应走到这里，调用方应在 isInitScripts() 为 true 后再调用
        daCritical << QObject::tr("DAPyScripts is not initialized, getDataProcess() called before initScripts()");  // cn:DAPyScripts 未初始化，在 initScripts() 之前调用了 getDataProcess()
        Q_ASSERT_X(s_models, "DAPyScripts::getDataProcess", "s_models is nullptr");
    }
    return s_models->workBench.getDataProcess();
}

void DAPyScripts::cleanup()
{
    s_models.reset();
}

}  // namespace DA
