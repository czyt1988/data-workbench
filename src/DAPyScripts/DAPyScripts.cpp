#include "DAPyScripts.h"
#include <QObject>
#include <QDebug>
#include "DAPyScriptsIO.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyInterpreter.h"

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
            qCritical() << QObject::tr("DAPyInterpreter is not initialized");  // cn:python环境未初始化
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
        qCritical() << QObject::tr("Initialized import scripts error:%1").arg(e.what());  // cn:初始化脚本失败
    }
    return false;
}

/**
 * @brief 获取io相关的script
 * @return
 */
DAPyScriptsIO& DAPyScripts::getIO()
{
    return s_models->workBench.getIO();
}

/**
 * @brief 获取dataframe相关的script
 * @return
 */
DAPyScriptsDataFrame& DAPyScripts::getDataFrame()
{
    return s_models->workBench.getDataFrame();
}

DAPyScriptsDataProcess& DAPyScripts::getDataProcess()
{
    return s_models->workBench.getDataProcess();
}

void DAPyScripts::cleanup()
{
    s_models.reset();
}

}  // namespace DA
