#include "Commands.h"
#include "DACoreInterface.h"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
/**
 * @brief 数据过滤
 *
 * 过滤DataFrame中的数据，根据索引列的值在lowervalue和uppervalue之间
 *
 * @param df  要过滤的DataFrame
 * @param lowervalue  最小值
 * @param uppervalue  最大值
 * @param index  索引列
 * @param par
 */
CommandDataFrame_filterByColumn::CommandDataFrame_filterByColumn(
    const DA::DAPyDataFrame& df, double lowervalue, double uppervalue, const QString& index, QUndoCommand* par
)
    : DA::DACommandWithTemporaryData(df, par)
    , DA::DACallBackInterface()
    , mlowervalue(lowervalue)
    , mUppervalue(uppervalue)
    , mIndex(index)
{
    setText(QObject::tr("Data Select"));  // cn:数据过滤
}

/**
 * @brief 撤销操作
 */
void CommandDataFrame_filterByColumn::undo()
{
    load();
    callback();
}

/**
 * @brief 执行过滤操作
 *
 * @return 执行是否成功
 */
bool CommandDataFrame_filterByColumn::exec()
{
    DA::DAPyScriptsDataFrame& pydf = DA::DAPyScripts::getDataFrame();
    if (!pydf.dataselect(dataframe(), mlowervalue, mUppervalue, mIndex)) {
        return false;
    }
    callback();
    return true;
}

//===================================================
// CommandDataFrame_querydatas
//===================================================
/**
 * @brief querydatas
 *
 * @param df  要查询的DataFrame
 * @param exper  查询表达式
 * @param par
 */
CommandDataFrame_querydatas::CommandDataFrame_querydatas(const DA::DAPyDataFrame& df, const QString& exper, QUndoCommand* par)
    : DA::DACommandWithTemporaryData(df, par), DA::DACallBackInterface(), mExper(exper)
{
    setText(QObject::tr("Data Query"));  // cn:数据查询
}

/**
 * @brief 撤销操作
 */
void CommandDataFrame_querydatas::undo()
{
    load();
    callback();
}

/**
 * @brief 执行查询操作
 *
 * @return 执行是否成功
 */
bool CommandDataFrame_querydatas::exec()
{
    DA::DAPyScriptsDataFrame& pydf = DA::DAPyScripts::getDataFrame();
    if (!pydf.queryDatas(dataframe(), mExper)) {
        return false;
    }
    callback();
    return true;
}

//===================================================
// CommandDataFrame_evalDatas
//===================================================
/**
 * @brief evaldatas
 *
 * 对DataFrame中的数据进行列运算
 *
 * @param df  要运算的DataFrame
 * @param exper  运算表达式
 * @param par
 */
CommandDataFrame_evalDatas::CommandDataFrame_evalDatas(const DA::DAPyDataFrame& df, const QString& exper, QUndoCommand* par)
    : DA::DACommandWithTemporaryData(df, par), DA::DACallBackInterface(), mExper(exper)
{
    setText(QObject::tr("Eval Data"));  // cn:列运算
}

/**
 * @brief 撤销操作
 */
void CommandDataFrame_evalDatas::undo()
{
    load();
    callback();
}

/**
 * @brief 执行列运算操作
 *
 * @return 执行是否成功
 */
bool CommandDataFrame_evalDatas::exec()
{
    DA::DAPyScriptsDataFrame& pydf = DA::DAPyScripts::getDataFrame();
    if (!pydf.evalDatas(dataframe(), mExper)) {
        return false;
    }
    callback();
    return true;
}
