#include "DAPyWorkBench.h"
#include "DALogCategory.h"

namespace DA
{

class DAPyWorkBench::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkBench)
public:
    PrivateData(DAPyWorkBench* p);
    DAPyScriptsIO mIO;
    DAPyScriptsDataFrame mDataframe;
    DAPyScriptsDataProcess mDataProcess;
    DAPyScriptsStatistics mStatistics;
};

DAPyWorkBench::PrivateData::PrivateData(DAPyWorkBench* p)
    : q_ptr(p), mIO(false), mDataframe(false), mDataProcess(false), mStatistics(false)
{
}

//===============================================================
// DAPyWorkBench
//===============================================================
DAPyWorkBench::DAPyWorkBench() : DAPyModule(), DA_PIMPL_CONSTRUCT
{
    import();
}

/** @brief 析构DAPyWorkBench */
DAPyWorkBench::~DAPyWorkBench()
{
}

/** @brief 导入DAWorkbench模块 */
bool DAPyWorkBench::import()
{
    bool res = DAPyModule::import("DAWorkbench");
    if (!res) {
        daCritical << QObject::tr("cannot import DAWorkbench module");  // cn:无法导入 DAWorkbench 模块
        return false;
    }
    // 调用 Python 端的 initialize()，初始化日志等子系统
    try {
        if (hasattr("initialize")) {
            attr("initialize")();
            qDebug() << "DAWorkbench.initialize() called successfully";
        } else {
            qWarning() << "DAWorkbench module has no initialize() function";
        }
    } catch (const std::exception& e) {
        qWarning() << "DAWorkbench.initialize() failed:" << e.what();
    }
    try {
        d_ptr->mIO.object()          = attr("io");
        d_ptr->mDataframe.object()   = attr("dataframe");
        d_ptr->mDataProcess.object() = attr("data_processing");
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
    // Statistics is a pure-Python subpackage, not re-exported by DAWorkbench.__init__,
    // so it imports itself directly rather than via attr().
    try {
        d_ptr->mStatistics.import();
    } catch (const std::exception& e) {
        qWarning() << "Failed to import DAStatistics:" << e.what();
    }
    return true;
}

/** @brief 获取IO脚本模块 @return IO脚本模块引用 */
DAPyScriptsIO& DAPyWorkBench::getIO()
{
    return d_ptr->mIO;
}

/** @brief 获取DataFrame脚本模块 @return DataFrame脚本模块引用 */
DAPyScriptsDataFrame& DAPyWorkBench::getDataFrame()
{
    return d_ptr->mDataframe;
}

/** @brief 获取数据处理脚本模块 @return 数据处理脚本模块引用 */
DAPyScriptsDataProcess& DAPyWorkBench::getDataProcess()
{
    return d_ptr->mDataProcess;
}

/** @brief 获取统计脚本模块 @return 统计脚本模块引用 */
DAPyScriptsStatistics& DAPyWorkBench::getStatistics()
{
    return d_ptr->mStatistics;
}

}
