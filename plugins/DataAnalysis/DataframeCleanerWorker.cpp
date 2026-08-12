#include "DataframeCleanerWorker.h"
#include <QUndoStack>
#include <QUndoGroup>
#include <QPointer>
#include "DAPyModule.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataOperateWidget.h"
#include "DADataUndoCommand.h"
#include "DACommandInterface.h"
#include "DADataOperateOfDataFrameWidget.h"

/**
 * @brief 构造函数
 * @param par 父对象
 */
DataframeCleanerWorker::DataframeCleanerWorker(QObject* par) : DataAnalysisBaseWorker(par)
{
    initializePythonEnv();
}

/**
 * @brief 析构函数
 */
DataframeCleanerWorker::~DataframeCleanerWorker()
{
}

/**
 * @brief 初始化Python环境
 * @return 初始化是否成功
 */
bool DataframeCleanerWorker::initializePythonEnv()
{
    try {
        DA::DAPyModule DADataAnalysisGui("DADataAnalysisGui");
        mDataCleanerModule  = std::make_unique< DA::DAPyModule >();
        *mDataCleanerModule = DADataAnalysisGui.attr("dataframe_cleaner");
        return true;
    } catch (const std::exception& e) {
        mDataCleanerModule.reset();
        qCritical() << e.what();
    }
    return false;
}

/**
 * @brief 删除缺失值
 */
void DataframeCleanerWorker::dropna()
{
    exec("dropna");
}

/**
 * @brief 删除重复值
 */
void DataframeCleanerWorker::drop_duplicates()
{
    exec("drop_duplicates");
}

/**
 * @brief 填充缺失值
 */
void DataframeCleanerWorker::fillna()
{
    exec("fillna");
}

/**
 * @brief 插值法填充缺失值
 */
void DataframeCleanerWorker::fill_interpolate()
{
    exec("fill_interpolate");
}

/**
 * @brief 基于IQR方法移除异常值
 */
void DataframeCleanerWorker::remove_outliers_iqr()
{
    exec("remove_outliers_iqr");
}

/**
 * @brief 基于Z-score方法移除异常值
 */
void DataframeCleanerWorker::remove_outliers_zscore()
{
    exec("remove_outliers_zscore");
}

/**
 * @brief 转换偏态数据以改善分布
 */
void DataframeCleanerWorker::transform_skewed_data()
{
    exec("transform_skewed_data");
}

/**
 * @brief 执行数据清洗函数
 * @param funname 函数名
 * @return 执行是否成功
 */
bool DataframeCleanerWorker::exec(const char* funname)
{
    try {
        auto fun = mDataCleanerModule->attr(funname);
        fun();
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
    return true;
}
