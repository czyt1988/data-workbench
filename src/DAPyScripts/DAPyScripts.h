#ifndef DAPYSCRIPTS_H
#define DAPYSCRIPTS_H
#include <QObject>
#include <QtCore/qglobal.h>
#include "DAPyScriptsGlobal.h"
#include "DAPyWorkBench.h"
#include "DAPyScriptsIO.h"
#include "DAPyScriptsDataFrame.h"
#include "DAPyScriptsDataProcess.h"
namespace DA
{
/**
 * @brief 这个是da外部脚本的总调度
 *
 * 此类在初始化的时候就会加载对应的脚本，da的业务逻辑将使用这些脚本进行
 *
 * @note 需要在DAPyInterpreter初始化之后调用此类，否则无法正常初始化
 */
class DAPYSCRIPTS_API DAPyScripts
{
public:
    DAPyScripts();
    ~DAPyScripts();
    // 是否初始化了脚本
    static bool isInitScripts();
    // 初始化脚本
    static bool initScripts();
    // 清理脚本模块，必须在Python解释器销毁之前调用
    static void cleanup();
    static DAPyScriptsIO& getIO();
    static DAPyScriptsDataFrame& getDataFrame();
    static DAPyScriptsDataProcess& getDataProcess();

protected:
    // 内部模块
    class InnerModules;
    static std::unique_ptr< InnerModules > s_models;
};
}  // namespace DA
#endif  // DAPYSCRIPTS_H
