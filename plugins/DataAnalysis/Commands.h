#ifndef COMMANDS_H
#define COMMANDS_H
#include <QUndoCommand>
#include "Commands/DACommandWithTemporaryData.h"
#include "pandas/DAPyDataFrame.h"
#include "DACallBackInterface.h"


/**
 * @brief 数据过滤
 *
 * 过滤DataFrame中的数据，根据索引列的值在lowervalue和uppervalue之间
 */
class CommandDataFrame_filterByColumn : public DA::DACommandWithTemporaryData, public DA::DACallBackInterface
{
public:
    CommandDataFrame_filterByColumn(
        const DA::DAPyDataFrame& df, double lowervalue, double uppervalue, const QString& index, QUndoCommand* par = nullptr
    );
    virtual void undo() override;
    virtual bool exec() override;

private:
    double mlowervalue { 0.0 };
    double mUppervalue { 0.0 };
    QString mIndex;
};


/**
 * @brief 数据查询
 *
 * 对DataFrame中的数据进行查询，根据表达式exper进行查询
 */
class CommandDataFrame_querydatas : public DA::DACommandWithTemporaryData, public DA::DACallBackInterface
{
public:
    CommandDataFrame_querydatas(const DA::DAPyDataFrame& df, const QString& exper, QUndoCommand* par = nullptr);
    virtual void undo() override;
    virtual bool exec() override;

private:
    QString mExper;
};


// CommandDataFrame_evalDatas 已按 004 计划收口移除（内核计算列命令替代，D0/R2）

#endif
