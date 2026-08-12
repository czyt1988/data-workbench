#ifndef DATAFRAMEIOWORKER_H
#define DATAFRAMEIOWORKER_H
#include <QObject>
#include <string>
#include "DataAnalysisBaseWorker.h"
class DataframeExportSettingsDialog;
class DataFrameExportRangeSelectDialog;
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
    ~DataframeIOWorker() override;
    bool initializePythonEnv();
public Q_SLOTS:
    // 导出单一数据
    void exportIndividualData();
    // 导出多个数据（将在python线程中进行）
    void exportMultipleData();
    // 导出到一个excel文件中
    void exportToOneExcelFile();
private Q_SLOTS:
    // 更新python的线程状态,msleep是给python线程的时间，evenTime是给自身主线程的时间
    void updatePythonThreadStatus(const std::string& taskid, int msleep = 500, int evenTime = 20);

private:
    std::unique_ptr< DA::DAPyModule > mDataAnalysisModule;
    std::unique_ptr< DA::DAPyModule > mThreadStatusMgrModule;
    // 导出多个数据
    DataframeExportSettingsDialog* mExportSettingDialog { nullptr };
    // 导出到excel
    DataFrameExportRangeSelectDialog* mExportRangeSelectDialog { nullptr };
};

#endif  // DATAFRAMEIOWORKER_H
