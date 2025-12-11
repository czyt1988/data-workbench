#ifndef DATAFRAMEIOWORKER_H
#define DATAFRAMEIOWORKER_H
#include <QObject>
#include "DataAnalysisBaseWorker.h"
/**
 * @brief 负责处理dataframe的io操作的工作者
 */
class DataframeIOWorker : public DataAnalysisBaseWorker
{
    Q_OBJECT
public:
    explicit DataframeIOWorker(QObject* par = nullptr);
    ~DataframeIOWorker();
public Q_SLOTS:
    // 导出单一数据
    void exportIndividualData();
    // 导出多个数据（将在python线程中进行）
    void exportMultipleData();

private:
};

#endif  // DATAFRAMEIOWORKER_H
