#ifndef DAPLOTUNDOCOMMANDS_H
#define DAPLOTUNDOCOMMANDS_H
#include <QUndoCommand>
#include <QPointer>
#include <QDateTime>
#include "DAFigureScrollArea.h"
namespace QIM
{
class QImFigureWidget;
class QImPlotNode;
}
/**
 *@file DAPlot的命令
 */
namespace DA
{

enum DAPlotCommandIDs
{
    PlotCommandID_Begin       = 1000,
    PlotCommandID_SubplotGrid = PlotCommandID_Begin + 1,
    PlotCommandID_End         = 5000,
};

/**
 * @brief 子图网格变更命令（支持比例变换合并）
 */
class DAPlotChangeSubplotGridCommand : public QUndoCommand
{
public:
    DAPlotChangeSubplotGridCommand(DAFigureScrollArea* figure,
                                   int newRows,
                                   int newCols,
                                   const std::vector< float >& newRowRatios    = std::vector< float >(),
                                   const std::vector< float >& newColumnRatios = std::vector< float >(),
                                   QUndoCommand* parent                        = nullptr);

    ~DAPlotChangeSubplotGridCommand() override;

    void redo() override;
    void undo() override;
    bool mergeWith(const QUndoCommand* command) override;
    int id() const override
    {
        return PlotCommandID_SubplotGrid;
    }  // 自定义命令ID，用于merge识别

private:
    // 判断是否为纯比例变换（行列数未变）
    bool isPureRatioChange() const;

private:
    QPointer< DAFigureScrollArea > m_figure;
    int m_oldRows { 1 };
    int m_oldCols { 1 };
    int m_newRows { 1 };
    int m_newCols { 1 };
    std::vector< float > m_oldRowRatios;
    std::vector< float > m_oldColumnRatios;
    std::vector< float > m_newRowRatios;
    std::vector< float > m_newColumnRatios;
    QDateTime m_timestamp;  // 命令创建时间戳
};

/**
 * @brief 针对添加绘图的命令
 */
class DAPlotAddPlotCommand : public QUndoCommand
{
public:
    DAPlotAddPlotCommand(DAFigureScrollArea* figure, QUndoCommand* parent = nullptr);
    DAPlotAddPlotCommand(DAFigureScrollArea* figure, QIM::QImPlotNode* plot, QUndoCommand* parent = nullptr);
    ~DAPlotAddPlotCommand() override;
    QIM::QImPlotNode* plotNode() const;
    void redo() override;
    void undo() override;

private:
    QPointer< DAFigureScrollArea > m_figure;
    QPointer< QIM::QImPlotNode > m_plot;
    int m_subplotIndex { -1 };  ///< 记录属于第几个subplot
    bool m_isNeedDelete { false };
};
}  // end namespace DA

#endif  // DAPLOTUNDOCOMMANDS_H
