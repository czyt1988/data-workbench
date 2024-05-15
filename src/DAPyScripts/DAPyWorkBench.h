#ifndef DAPYWORKBENCH_H
#define DAPYWORKBENCH_H
#include "DAPyScriptsGlobal.h"
#include "DAPyModule.h"
#include <QVariant>
#include "DAPyScriptsIO.h"
#include "DAPyScriptsDataFrame.h"
#include "DAPyScriptsDataProcess.h"
namespace DA
{
class DAPYSCRIPTS_API DAPyWorkBench : public DAPyModule
{
	DA_DECLARE_PRIVATE(DAPyWorkBench)
public:
	DAPyWorkBench();
	~DAPyWorkBench();
	// 引入
	bool import();
	// io模块
	DAPyScriptsIO& getIO();
	DAPyScriptsDataFrame& getDataFrame();
	DAPyScriptsDataProcess& getDataProcess();
};
}
#endif  // DAPYWORKBENCH_H
