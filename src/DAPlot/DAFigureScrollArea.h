#ifndef DAFIGURESCROLLAREA_H
#define DAFIGURESCROLLAREA_H
#include "DAPlotAPI.h"
#include <QScrollArea>
class QUndoStack;
namespace QIM
{
class QImFigureWidget;
class QImPlotNode;
}
namespace DA
{
/**
 * @brief 用于展示绘图的滚动区域
 *
 * @note 末尾带“_”的函数是支持redo/undo的函数
 */
class DAPLOT_API DAFigureScrollArea : public QScrollArea
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureScrollArea)
public:
    explicit DAFigureScrollArea(QWidget* parent = nullptr);
    ~DAFigureScrollArea();
    // 获取绘图窗口
    QIM::QImFigureWidget* figure() const;
    // id
    QString getFigureId() const;
    void setFigureId(const QString& id);
    // 创建绘图
    QIM::QImPlotNode* createPlot();
    // 回退栈
    QUndoStack* undoStack() const;
    //----------------------------------------------------
    // subplot
    //----------------------------------------------------
    void setSubplotGrid(int rows,
                        int cols,
                        const std::vector< float >& rowsRatios = std::vector< float >(),
                        const std::vector< float >& colsRatios = std::vector< float >());
    void setSubplotGrid_(int rows,
                         int cols,
                         const std::vector< float >& rowsRatios = std::vector< float >(),
                         const std::vector< float >& colsRatios = std::vector< float >());
    // 获取subplot的尺寸,width=columns,height = rows
    int subplotGridRows() const;
    int subplotGridColumns() const;
    std::vector< float > subplotGridRowRatios() const;
    std::vector< float > subplotGridColumnRatios() const;
    //----------------------------------------------------
    // plot
    //----------------------------------------------------
    // 创建并添加到节点树中
    QIM::QImPlotNode* createPlotNode();
    QIM::QImPlotNode* createPlotNode_();
    // 获取所有绘图节点
    QList< QIM::QImPlotNode* > plotNodes() const;
    // 绘图的数量
    int plotCount() const;
    void addPlotNode(QIM::QImPlotNode* plot);
    void addPlotNode_(QIM::QImPlotNode* plot);
    // 插入绘图，注意plotIndex是subplot节点下面绘图节点的索引，其它节点会跳过,plotIndex可以是-1，则代表在最前面插入，可以大于等于size，代表最后插入
    void insertPlotNode(int plotIndex, QIM::QImPlotNode* plot);
    // plotNode在subplot下的索引
    int plotNodeSubplotIndex(QIM::QImPlotNode* plot);
    // 提取出QImPlotNode，不在此figure里管理
    bool takePlotNode(QIM::QImPlotNode* plot);
    // 移除绘图，plot会被删除
    void removePlotNode(QIM::QImPlotNode* plot);
    // 设置为当前绘图,可设置为nullptr代表无当前绘图
    void setCurrentPlot(QIM::QImPlotNode* plot);
    QIM::QImPlotNode* getCurrentPlot() const;
    //----------------------------------------------------
    // 自动track信号到undostack
    //----------------------------------------------------
    void setAutoTrackFigureChangedToUndoCommand(bool on);
    bool isAutoTrackFigureChangedToUndoCommand() const;

private Q_SLOTS:
    // 子图grid信息发生改变
    void onSubplotGridChanged();

private:
    void init();
};
}
#endif  // DAFIGURESCROLLAREA_H
