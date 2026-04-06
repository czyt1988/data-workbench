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
class DataFrameDataSearchDialog;
class DataFrameDataSelectDialog;
class DataFrameSortDialog;
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
    ~DataframeOperateWorker();
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
    // 检索给定的数据
    void searchData();
    // 过滤给定条件外的数据
    void filterByColumn();
    QUndoCommand* filterByColumn(
        const DA::DAPyDataFrame& df, double lower = 0.0, double upper = 0.0, const QString& index = QString(), Callback fp = nullptr
    );
    // 数据排序
    void sortDatas();
    QUndoCommand* sortDatas(const DA::DAPyDataFrame& df, const QString& by, const bool ascending, Callback fp = nullptr);

private:
    std::unique_ptr< DA::DAPyModule > m_dataOperateModule;
    DataFrameDataSearchDialog* m_searchDialog { nullptr };
    DataFrameDataSelectDialog* m_selectDialog { nullptr };
    DataFrameSortDialog* m_sortDialog { nullptr };
    DataFrameQueryDatasDialog* m_queryDatasDialog { nullptr };
    DataFrameEvalDatasDialog* m_evalDatasDialog { nullptr };
    DataFrameCreatePivotTableDialog* m_pivotTableDialog { nullptr };
};

#endif  // DATAFRAMEOPERATEWORKER_H
