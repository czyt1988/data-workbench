#include "DAPyWorkBench.h"

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
};

DAPyWorkBench::PrivateData::PrivateData(DAPyWorkBench* p) : q_ptr(p), mIO(false), mDataframe(false), mDataProcess(false)
{
}

//===============================================================
// DAPyWorkBench
//===============================================================
DAPyWorkBench::DAPyWorkBench() : DAPyModule(), DA_PIMPL_CONSTRUCT
{
    import();
}

DAPyWorkBench::~DAPyWorkBench()
{
}

bool DAPyWorkBench::import()
{
    bool res = DAPyModule::import("DAWorkbench");
    if (!res) {
        qCritical() << QObject::tr("can not import DAWorkbench module");
    }
    try {
        d_ptr->mIO.object()          = attr("io");
        d_ptr->mDataframe.object()   = attr("dataframe");
        d_ptr->mDataProcess.object() = attr("data_processing");
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
    return true;
}

DAPyScriptsIO& DAPyWorkBench::getIO()
{
    return d_ptr->mIO;
}

DAPyScriptsDataFrame& DAPyWorkBench::getDataFrame()
{
    return d_ptr->mDataframe;
}

DAPyScriptsDataProcess& DAPyWorkBench::getDataProcess()
{
    return d_ptr->mDataProcess;
}

}
