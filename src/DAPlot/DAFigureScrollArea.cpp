#include "DAFigureScrollArea.h"
#include <QUndoStack>
#include <QUuid>
#include <QPointer>
// DAPlot
#include "DAPlotUndoCommands.h"
// QIm
#include "QImFigureWidget.h"
#include "plot/QImPlotNode.h"
#include "plot/QImSubplotsNode.h"
namespace DA
{
class DAFigureScrollArea::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigureScrollArea)
public:
    PrivateData(DAFigureScrollArea* p);
    QIM::QImSubplotsNode* subplotNode() const;

public:
    QString id;
    QUndoStack undoStack;  ///<
    QIM::QImFigureWidget* figure { nullptr };
    bool autoTrackFigureChangedToUndoCommand { false };  ///< 跟踪figure自身的信号变化到命令中
    QPointer< QIM::QImPlotNode > currentPlot;
};

DAFigureScrollArea::PrivateData::PrivateData(DAFigureScrollArea* p) : q_ptr(p)
{
    id = QUuid::createUuid().toString();
}

QIM::QImSubplotsNode* DAFigureScrollArea::PrivateData::subplotNode() const
{
    return this->figure->subplotNode();
}

//----------------------------------------------------
// DAFigureScrollArea
//----------------------------------------------------

DAFigureScrollArea::DAFigureScrollArea(QWidget* parent) : QScrollArea(parent), DA_PIMPL_CONSTRUCT
{
}

DAFigureScrollArea::~DAFigureScrollArea()
{
}

void DAFigureScrollArea::init()
{
    DA_D(d);
    setWindowIcon(QIcon(":/DAPlot/icon/icon/figure.svg"));
    d->figure = new QIM::QImFigureWidget();
    setWidget(d->figure);
    setAutoTrackFigureChangedToUndoCommand(true);
}

QIM::QImFigureWidget* DAFigureScrollArea::figure() const
{
    return d_ptr->figure;
}

QString DAFigureScrollArea::getFigureId() const
{
    return d_ptr->id;
}

void DAFigureScrollArea::setFigureId(const QString& id)
{
    d_ptr->id = id;
}

QIM::QImPlotNode* DAFigureScrollArea::createPlot()
{
    return d_ptr->figure->createPlotNode();
}

QUndoStack* DAFigureScrollArea::undoStack() const
{
    return &(d_ptr->undoStack);
}

void DAFigureScrollArea::setSubplotGrid(int rows,
                                        int cols,
                                        const std::vector< float >& rowsRatios,
                                        const std::vector< float >& colsRatios)
{
    d_ptr->figure->setSubplotGrid(rows, cols, rowsRatios, colsRatios);
}

void DAFigureScrollArea::setSubplotGrid_(int rows,
                                         int cols,
                                         const std::vector< float >& rowsRatios,
                                         const std::vector< float >& colsRatios)
{
    if (rows < 1 || cols < 1) {
        return;
    }
    DAPlotChangeSubplotGridCommand* cmd = new DAPlotChangeSubplotGridCommand(this, rows, cols, rowsRatios, colsRatios);
    d_ptr->undoStack.push(cmd);
}

int DAFigureScrollArea::subplotGridRows() const
{
    return d_ptr->figure->subplotGridRows();
}

int DAFigureScrollArea::subplotGridColumns() const
{
    return d_ptr->figure->subplotGridColumns();
}

std::vector< float > DAFigureScrollArea::subplotGridRowRatios() const
{
    return d_ptr->figure->subplotGridRowRatios();
}

std::vector< float > DAFigureScrollArea::subplotGridColumnRatios() const
{
    return d_ptr->figure->subplotGridColumnRatios();
}

QIM::QImPlotNode* DAFigureScrollArea::createPlotNode()
{
    return d_ptr->figure->createPlotNode();
}

QIM::QImPlotNode* DAFigureScrollArea::createPlotNode_()
{
    DAPlotAddPlotCommand* cmd = new DAPlotAddPlotCommand(this);
    d_ptr->undoStack.push(cmd);
    return cmd->plotNode();
}

QList< QIM::QImPlotNode* > DAFigureScrollArea::plotNodes() const
{
    return d_ptr->figure->plotNodes();
}

int DAFigureScrollArea::plotCount() const
{
    return d_ptr->figure->plotCount();
}

void DAFigureScrollArea::addPlotNode(QIM::QImPlotNode* plot)
{
    d_ptr->figure->addPlotNode(plot);
}

void DAFigureScrollArea::addPlotNode_(QIM::QImPlotNode* plot)
{
    DAPlotAddPlotCommand* cmd = new DAPlotAddPlotCommand(this, plot);
    d_ptr->undoStack.push(cmd);
}

void DAFigureScrollArea::insertPlotNode(int plotIndex, QIM::QImPlotNode* plot)
{
    d_ptr->figure->insertPlotNode(plotIndex, plot);
}

int DAFigureScrollArea::plotNodeSubplotIndex(QIM::QImPlotNode* plot)
{
    return d_ptr->figure->plotNodeSubplotIndex(plot);
}

bool DAFigureScrollArea::takePlotNode(QIM::QImPlotNode* plot)
{
    return d_ptr->figure->takePlotNode(plot);
}

void DAFigureScrollArea::removePlotNode(QIM::QImPlotNode* plot)
{
    d_ptr->figure->removePlotNode(plot);
}

void DAFigureScrollArea::setCurrentPlot(QIM::QImPlotNode* plot)
{
    d_ptr->currentPlot = plot;
}

QIM::QImPlotNode* DAFigureScrollArea::getCurrentPlot() const
{
    return d_ptr->currentPlot.data();
}

/**
 * @brief 设置此命令后，一些命令会自动创建命令
 *
 * 涉及命令如下：QImSubplotsNode::gridInfoChanged
 * - QImSubplotsNode::gridInfoChanged
 * @param on
 */
void DAFigureScrollArea::setAutoTrackFigureChangedToUndoCommand(bool on)
{
    DA_D(d);
    d->autoTrackFigureChangedToUndoCommand = on;
    QIM::QImSubplotsNode* subplot          = d->figure->subplotNode();
    if (subplot) {
        connect(subplot, &QIM::QImSubplotsNode::gridInfoChanged, this, &DAFigureScrollArea::onSubplotGridChanged);
    } else {
        disconnect(subplot, &QIM::QImSubplotsNode::gridInfoChanged, this, &DAFigureScrollArea::onSubplotGridChanged);
    }
}

bool DAFigureScrollArea::isAutoTrackFigureChangedToUndoCommand() const
{
    return d_ptr->autoTrackFigureChangedToUndoCommand;
}

void DAFigureScrollArea::onSubplotGridChanged()
{
    if (!isAutoTrackFigureChangedToUndoCommand()) {
        return;
    }
    DA_D(d);
    QIM::QImSubplotsNode* subplot = d->subplotNode();
    if (!subplot) {
        return;
    }
    DAPlotChangeSubplotGridCommand* cmd = new DAPlotChangeSubplotGridCommand(
        this, subplot->rows(), subplot->columns(), subplot->rowRatios(), subplot->columnRatios());
    d->undoStack.push(cmd);
}

}
