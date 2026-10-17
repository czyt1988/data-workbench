#ifndef DATAFRAMEOPERATEWORKER_H
#define DATAFRAMEOPERATEWORKER_H
#include "DataAnalysisBaseWorker.h"
#include <functional>
#include "pandas/DAPyDataFrame.h"
namespace DA
{
class DAPyModule;
}

class QUndoCommand;
class DataFrameDataSelectDialog;
class DataFrameQueryDatasDialog;
class DataFrameEvalDatasDialog;
class DataFrameCreatePivotTableDialog;

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
    ~DataframeOperateWorker() override;
    bool initializePythonEnv();
public Q_SLOTS:
    // 创建数据描述
    void createDataframeDescribe();
    // 创建数据透视表
    void createPivotTable();
    DA::DAPyDataFrame createPivotTable(
        const DA::DAPyDataFrame& df,
        const QStringList value    = QStringList(),
        const QStringList index    = QStringList(),
        const QStringList columns  = QStringList(),
        const QString& aggfunc     = QStringLiteral("mean"),
        bool margins               = false,
        const QString& marginsName = QStringLiteral("All"),
        bool sort                  = false
    );
    // 列运算
    void evalDatas();
    QUndoCommand* evalDatas(const DA::DAPyDataFrame& df, const QString& exper, Callback fp = nullptr);
    // 过滤给定条件外的数据
    void queryDatas();
    QUndoCommand* queryDatas(const DA::DAPyDataFrame& df, const QString& exper, Callback fp = nullptr);
    // 过滤给定条件外的数据
    void filterByColumn();
    QUndoCommand* filterByColumn(
        const DA::DAPyDataFrame& df, double lower = 0.0, double upper = 0.0, const QString& index = QString(), Callback fp = nullptr
    );
    // 排序/检索入口已按 002 计划收口移除（内核表头点击排序 + 浮动查找条替代）；
    // DADataAnalysisCore 与工作流节点 data_sort_node.py/data_search_node.py 是独立入口，保留

private:
    std::unique_ptr< DA::DAPyModule > mDataOperateModule;
    DataFrameDataSelectDialog* mSelectDialog { nullptr };
    DataFrameQueryDatasDialog* mQueryDatasDialog { nullptr };
    DataFrameEvalDatasDialog* mEvalDatasDialog { nullptr };
    DataFrameCreatePivotTableDialog* mPivotTableDialog { nullptr };
};

#endif  // DATAFRAMEOPERATEWORKER_H
