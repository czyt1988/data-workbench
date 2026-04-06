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
    setText(QObject::tr("data select"));  // cn:数据过滤
}

void CommandDataFrame_filterByColumn::undo()
{
    load();
    callback();
}

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
// CommandDataFrame_sort
//===================================================

/**
 * @brief 数据排序
 *
 * 对DataFrame中的数据进行排序
 *
 * @param df  要排序的DataFrame
 * @param by  排序的列名
 * @param ascending 是否升序
 * @param par
 */
CommandDataFrame_sort::CommandDataFrame_sort(const DA::DAPyDataFrame& df, const QString& by, const bool ascending, QUndoCommand* par)
    : DA::DACommandWithTemporaryData(df, par), DA::DACallBackInterface(), mBy(by), mAscending(ascending)
{
    setText(QObject::tr("data sort"));  // cn:数据排序
}

void CommandDataFrame_sort::undo()
{
    load();
    callback();
}

bool CommandDataFrame_sort::exec()
{
    DA::DAPyScriptsDataFrame& pydf = DA::DAPyScripts::getDataFrame();
    if (!pydf.sort(dataframe(), mBy, mAscending)) {
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
    setText(QObject::tr("data query"));  // cn:数据查询
}

void CommandDataFrame_querydatas::undo()
{
    load();
    callback();
}

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
    setText(QObject::tr("eval datas"));  // cn:列运算
}

void CommandDataFrame_evalDatas::undo()
{
    load();
    callback();
}

bool CommandDataFrame_evalDatas::exec()
{
    DA::DAPyScriptsDataFrame& pydf = DA::DAPyScripts::getDataFrame();
    if (!pydf.evalDatas(dataframe(), mExper)) {
        return false;
    }
    callback();
    return true;
}
