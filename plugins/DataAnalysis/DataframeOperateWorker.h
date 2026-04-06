#ifndef DATAFRAMEOPERATEWORKER_H
#define DATAFRAMEOPERATEWORKER_H
#include "DataAnalysisBaseWorker.h"
#include <functional>
namespace DA
{
class DAPyModule;
}
class QUndoCommand;
class DataFrameDataSearchDialog;
class DataFrameDataSelectDialog;
class DataFrameSortDialog;
class DataFrameQueryDatasDialog;
class DataFrameEvalDatasDialog;
/**
 * @brief 负责数据操作相关业务
 */
class DataframeOperateWorker : public DataAnalysisBaseWorker
{
    Q_OBJECT
public:
    using Callback = std::function< void() >;

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
    QUndoCommand* evalDatas(const DAPyDataFrame& df, const QString& exper, Callback fp = nullptr);
    // 过滤给定条件外的数据
    void queryDatas();
    QUndoCommand* queryDatas(const DAPyDataFrame& df, const QString& exper, Callback fp = nullptr);
    // 检索给定的数据
    void searchData();
    // 过滤给定条件外的数据
    void filterByColumn();
    QUndoCommand* filterByColumn(
        const DA::DAPyDataFrame& df, double lower = 0.0, double upper = 0.0, const QString& index = QString(), Callback fp = nullptr
    );
    // 数据排序
    void sortDatas();
    QUndoCommand* sortDatas(const DAPyDataFrame& df, const QString& by, const bool ascending, Callback fp = nullptr);

private:
    std::unique_ptr< DA::DAPyModule > m_dataOperateModule;
    DataFrameDataSearchDialog* m_searchDialog { nullptr };
    DataFrameDataSelectDialog* m_selectDialog { nullptr };
    DataFrameSortDialog* m_sortDialog { nullptr };
    DataFrameQueryDatasDialog* m_queryDatasDialog { nullptr };
    DataFrameEvalDatasDialog* m_evalDatasDialog { nullptr };
};

#endif  // DATAFRAMEOPERATEWORKER_H
