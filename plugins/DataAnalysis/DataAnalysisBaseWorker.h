#ifndef DATAANALYSISBASEWORKER_H
#define DATAANALYSISBASEWORKER_H
#include <QObject>
#include <QMainWindow>
#include "DAInterfaceHelper.h"
namespace DA
{
class DACoreInterface;
class DAUIInterface;
class DADockingAreaInterface;
class DADataManagerInterface;
class DACommandInterface;
class DAProjectInterface;
}

/**
 * @brief 本插件的基础工作类
 */
class DataAnalysisBaseWorker : public QObject, public DA::DAInterfaceHelper
{
    Q_OBJECT
public:
    explicit DataAnalysisBaseWorker(QObject* par = nullptr);
    ~DataAnalysisBaseWorker();
    virtual void initialize(DA::DACoreInterface* core);
};

#endif  // DATAANALYSISBASEWORKER_H
