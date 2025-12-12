#ifndef DATAFRAMECLEANERWORKER_H
#define DATAFRAMECLEANERWORKER_H
#include "DataAnalysisBaseWorker.h"
namespace DA
{
class DAPyModule;
}
/**
 * @brief 负责数据清洗相关业务
 */
class DataframeCleanerWorker : public DataAnalysisBaseWorker
{
    Q_OBJECT
public:
    explicit DataframeCleanerWorker(QObject* par = nullptr);
    ~DataframeCleanerWorker();
    bool initializePythonEnv();
public Q_SLOTS:
private:
    std::unique_ptr< DA::DAPyModule > m_dataAnalysisModule;
};

#endif  // DATAFRAMECLEANERWORKER_H
