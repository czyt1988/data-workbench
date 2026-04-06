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
 * @note 不建议在插件中创建DAPy相关的类，因为插件析构的顺序目前没有做保证，有可能python在插件之前析构而导致问题
 */
class DAPYSCRIPTS_API DAPyScripts
{
    DA_DECLARE_PRIVATE(DAPyScripts)

public:
    DAPyScripts();
    ~DAPyScripts();
    // 初始化脚本
    void appendSysPath(const QString& path);
    bool isInitScripts() const;

    DAPyScriptsIO& getIO();
    DAPyScriptsDataFrame& getDataFrame();
    DAPyScriptsDataProcess& getDataProcess();

protected:
    // import sys
    bool loadSysModule();
};
}  // namespace DA
#endif  // DAPYSCRIPTS_H
