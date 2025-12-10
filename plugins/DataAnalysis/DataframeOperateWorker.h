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
    // 创建数据描述
    void createDataframeDescribe();
    // 创建数据透视表
    void createPivotTable();
    // 列运算
    void evalDatas();
    // 过滤给定条件外的数据
    void queryDatas();
    // 检索给定的数据
    void searchData();
    // 过滤给定条件外的数据
    void filterByColumn();
    // 数据排序
    void sortDatas();

private:
    std::unique_ptr< DA::DAPyModule > m_dataOperateModule;
};

#endif  // DATAFRAMEOPERATEWORKER_H
