#ifndef DATAFRAMEOPERATEWORKER_H
#define DATAFRAMEOPERATEWORKER_H
#include "DataAnalysisBaseWorker.h"
namespace DA
{
class DAPyModule;
}

/**
 * @brief 负责数据操作相关业务
 */
class DataframeOperateWorker : public DataAnalysisBaseWorker
{
    Q_OBJECT
public:
    explicit DataframeOperateWorker(QObject* par = nullptr);
    ~DataframeOperateWorker();
    bool initializePythonEnv();
public Q_SLOTS:
    void createDataframeDescribe();

private:
    std::unique_ptr< DA::DAPyModule > m_dataOperateModule;
};

#endif  // DATAFRAMEOPERATEWORKER_H
