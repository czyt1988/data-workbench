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
    // 删除异常值
    void dropna();
    // 删除重复值
    void drop_duplicates();
    // 填充缺失值
    void fillna();
    // 插值填充
    void fill_interpolate();
    // IQR 删除异常值
    void remove_outliers_iqr();
    // Z-Score 替换异常值
    void remove_outliers_zscore();
    // 转换偏态数据
    void transform_skewed_data();

private:
    // 执行函数
    bool exec(const char* funname);

private:
    std::unique_ptr< DA::DAPyModule > m_dataCleanerModule;
};

#endif  // DATAFRAMECLEANERWORKER_H
