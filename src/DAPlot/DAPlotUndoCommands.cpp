#include "DAPlotUndoCommands.h"
#include <QtMath>
#include "plot/QImPlotNode.h"
namespace DA
{
//----------------------------------------------------
// DAPlotChangeSubplotGridCommand
//----------------------------------------------------

DAPlotChangeSubplotGridCommand::DAPlotChangeSubplotGridCommand(DAFigureScrollArea* figure,
                                                               int newRows,
                                                               int newCols,
                                                               const std::vector< float >& newRowRatios,
                                                               const std::vector< float >& newColumnRatios,
                                                               QUndoCommand* parent)
    : QUndoCommand(parent), m_figure(figure), m_timestamp(QDateTime::currentDateTime())
{
    if (m_figure) {
        m_oldRows         = figure->subplotGridRows();
        m_oldCols         = figure->subplotGridColumns();
        m_oldRowRatios    = figure->subplotGridRowRatios();
        m_oldColumnRatios = figure->subplotGridColumnRatios();
    }
    m_newRows         = newRows;
    m_newCols         = newCols;
    m_newRowRatios    = newRowRatios;
    m_newColumnRatios = newColumnRatios;
}

DAPlotChangeSubplotGridCommand::~DAPlotChangeSubplotGridCommand()
{
}

bool DAPlotChangeSubplotGridCommand::isPureRatioChange() const
{
    return (m_oldRows == m_newRows) && (m_oldCols == m_newCols);
}

void DAPlotChangeSubplotGridCommand::redo()
{
    if (m_figure) {
        if (!m_newRowRatios.empty() || !m_newColumnRatios.empty()) {
            m_figure->setSubplotGrid(m_newRows, m_newCols, m_newRowRatios, m_newColumnRatios);
        } else {
            m_figure->setSubplotGrid(m_newRows, m_newCols);
        }
    }
}

void DAPlotChangeSubplotGridCommand::undo()
{
    if (m_figure) {
        if (!m_oldRowRatios.empty() || !m_oldColumnRatios.empty()) {
            m_figure->setSubplotGrid(m_oldRows, m_oldCols, m_oldRowRatios, m_oldColumnRatios);
        } else {
            m_figure->setSubplotGrid(m_oldRows, m_oldCols);
        }
    }
}

bool DAPlotChangeSubplotGridCommand::mergeWith(const QUndoCommand* command)
{
    if (command->id() != id()) {
        return false;
    }

    const DAPlotChangeSubplotGridCommand* other = dynamic_cast< const DAPlotChangeSubplotGridCommand* >(command);
    if (!other || !m_figure || (m_figure != other->m_figure)) {
        return false;
    }

    // 行列变换命令禁止合并
    if (!isPureRatioChange() && (m_newRows != other->m_newRows || m_newCols != other->m_newCols)) {
        return false;
    }

    // 时间窗口检查：超过3分钟不合并
    if (qAbs(m_timestamp.secsTo(other->m_timestamp)) > 180) {
        return false;
    }

    // 合并：保留初始状态（用于undo），仅更新目标比例和时间戳
    m_newRowRatios    = other->m_newRowRatios;
    m_newColumnRatios = other->m_newColumnRatios;
    m_timestamp       = other->m_timestamp;  // 更新为最新操作时间

    return true;
}

//----------------------------------------------------
// DAPlotAddPlotCommand
//----------------------------------------------------
DAPlotAddPlotCommand::DAPlotAddPlotCommand(DAFigureScrollArea* figure, QUndoCommand* parent)
    : QUndoCommand(parent), m_figure(figure)
{
}

DAPlotAddPlotCommand::DAPlotAddPlotCommand(DAFigureScrollArea* figure, QIM::QImPlotNode* plot, QUndoCommand* parent)
    : QUndoCommand(parent), m_figure(figure), m_plot(plot)
{
    // 要获取m_subplotIndex的位置
    m_subplotIndex = figure->plotCount();
}

DAPlotAddPlotCommand::~DAPlotAddPlotCommand()
{
    if (m_isNeedDelete && m_plot) {
        m_plot->deleteLater();
        m_plot.clear();
    }
}

QIM::QImPlotNode* DAPlotAddPlotCommand::plotNode() const
{
    return m_plot.data();
}

void DAPlotAddPlotCommand::redo()
{
    if (m_figure) {
        if (!m_plot) {
            // 首次添加
            m_plot = m_figure->createPlot();
            // 同步获取index
            m_subplotIndex = m_figure->plotNodeSubplotIndex(m_plot.data());
        } else {
            // redo
            if (m_subplotIndex >= 0) {
                m_figure->insertPlotNode(m_subplotIndex, m_plot.data());
            } else {
                m_figure->addPlotNode(m_plot.data());
            }
        }
        m_isNeedDelete = false;
    }
}

void DAPlotAddPlotCommand::undo()
{
    if (m_figure && m_plot) {
        m_figure->takePlotNode(m_plot.data());
        m_isNeedDelete = true;
    }
}

}
