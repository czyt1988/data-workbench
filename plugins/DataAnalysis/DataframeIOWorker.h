#ifndef DATAFRAMEIOWORKER_H
#define DATAFRAMEIOWORKER_H
#include <QObject>
#include <string>
#include "DataAnalysisBaseWorker.h"
class DataframeExportSettingsDialog;
namespace DA
{
class DAPyModule;
}
/**
 * @brief 负责处理dataframe的io操作的工作者
 */
class DataframeIOWorker : public DataAnalysisBaseWorker
{
    Q_OBJECT
public:
    explicit DataframeIOWorker(QObject* par = nullptr);
    ~DataframeIOWorker();
    bool initializePythonEnv();
public Q_SLOTS:
    // 导出单一数据
    void exportIndividualData();
    // 导出多个数据（将在python线程中进行）
    void exportMultipleData();
private Q_SLOTS:
    // 进行exportMultipleData任务进度
    void onExportMultipleDataTask();

private:
    std::unique_ptr< DA::DAPyModule > m_dataAnalysisModule;
    std::unique_ptr< DA::DAPyModule > m_threadStatusMgrModule;
    DataframeExportSettingsDialog* m_exportSettingDialog { nullptr };
    std::string m_exportDatasTaskID;  ///< 记录导出数据的任务id
};

#endif  // DATAFRAMEIOWORKER_H
