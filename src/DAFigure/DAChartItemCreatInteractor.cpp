#include "DAChartItemCreatInteractor.h"
#include <QMouseEvent>
#include <QPen>
#include <QDebug>
#include "DAChartWidget.h"
#include "DADataProbeMarker.h"
#include "da_qt5qt6_compat.hpp"

#include "qwt_plot_item.h"
#include "qwt_plot_marker.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
namespace DA
{
QwtPlotMarker* createMarkerPlotItem(const QPointF& pos, const QString& title, const QPen& pen, QwtPlotMarker::LineStyle lineStyle);

class DAChartItemCreatInteractor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartItemCreatInteractor)
public:
    PrivateData(DAChartItemCreatInteractor* p);
    ~PrivateData();
    void releaseTmpItem();
    FpCreatePlotItem mCreatePlotItem { nullptr };
    QwtPlotItem* mTmpItem { nullptr };
};

/**
 * @brief 构造函数
 * @param p 父对象指针
 */
DAChartItemCreatInteractor::PrivateData::PrivateData(DAChartItemCreatInteractor* p)
{
}

/**
 * @brief 析构函数
 */
DAChartItemCreatInteractor::PrivateData::~PrivateData()
{
    releaseTmpItem();
}

/**
 * @brief 释放临时图元项
 */
void DAChartItemCreatInteractor::PrivateData::releaseTmpItem()
{
    if (mTmpItem) {
        mTmpItem->detach();
        delete mTmpItem;
        mTmpItem = nullptr;
    }
}
//==============================================
// DAChartItemCreatInteractor
//==============================================
/**
 * @brief 构造函数
 * @param parent 父QwtPlot对象
 * @param fun 创建图元项的工厂函数
 */
DAChartItemCreatInteractor::DAChartItemCreatInteractor(QwtPlot* parent, FpCreatePlotItem fun)
    : DAAbstractChartEditor(parent), DA_PIMPL_CONSTRUCT
{
    setPlotItemInteractorFactory(fun);
}

/**
 * @brief 析构函数
 */
DAChartItemCreatInteractor::~DAChartItemCreatInteractor()
{
}

/**
 * @brief 设置创建图元项的工厂函数
 * @param fun 工厂函数指针
 */
void DAChartItemCreatInteractor::setPlotItemInteractorFactory(FpCreatePlotItem fun)
{
    d_ptr->mCreatePlotItem = fun;
}

/**
 * @brief 获取创建图元项的工厂函数
 * @return 工厂函数指针
 */
DAChartItemCreatInteractor::FpCreatePlotItem DAChartItemCreatInteractor::getPlotItemInteractorFactory() const
{
    return d_ptr->mCreatePlotItem;
}

/**
 * @brief 运行时类型标识
 * @return 类型标识值
 */
int DAChartItemCreatInteractor::rtti() const
{
    return DAAbstractChartEditor::RTTICreatInteractor;
}

/**
 * @brief 提取临时图元项的所有权
 * @return 临时图元项指针，调用后内部指针置空
 */
QwtPlotItem* DA::DAChartItemCreatInteractor::takeItem()
{
    DA_D(d);
    QwtPlotItem* item = d->mTmpItem;
    d->mTmpItem       = nullptr;
    return item;
}

/**
 * @brief 鼠标按下事件处理
 * @param e 鼠标事件
 * @return 是否处理事件
 */
bool DAChartItemCreatInteractor::mousePressEvent(const QMouseEvent* e)
{
    if (d_ptr->mCreatePlotItem) {
        QwtPlot* gca = plot();
        if (!gca) {
            return false;
        }
        Q_EMIT beginEdit();
        QPoint canvasPos = compat::eventPos(e);
        QwtScaleMap xMap = gca->canvasMap(gca->visibleXAxisId());
        QwtScaleMap yMap = gca->canvasMap(gca->visibleYAxisId());
        QPointF pos      = QPointF(xMap.invTransform(canvasPos.x()), yMap.invTransform(canvasPos.y()));

        QwtPlotItem* item = d_ptr->mCreatePlotItem(gca, pos);
        if (item) {
            d_ptr->mTmpItem = item;
            QwtPlot* p      = item->plot();
            if (p) {
                p->replot();
            }
        }
        Q_EMIT finishedEdit(false);
    }
    return false;
}

/**
 * @brief 创建标记图元项
 * @param pos 坐标位置
 * @param title 标题
 * @param pen 画笔
 * @param lineStyle 线条样式
 * @return 创建的QwtPlotMarker指针
 */
QwtPlotMarker* createMarkerPlotItem(const QPointF& pos, const QString& title, const QPen& pen, QwtPlotMarker::LineStyle lineStyle)
{
    QwtPlotMarker* marker = new QwtPlotMarker(title);
    marker->setLineStyle(lineStyle);
    marker->setLinePen(pen);
    marker->setValue(pos);
    return marker;
}

/**
 * @brief 创建水平线标记图元项
 * @param plot 关联的QwtPlot
 * @param pos 坐标位置
 * @return 创建的QwtPlotItem指针
 */
QwtPlotItem* createHLineMarkerPlotItem(QwtPlot* plot, const QPointF& pos)
{
    QwtPlotMarker* marker = createMarkerPlotItem(
        pos,
        QObject::tr("Horizontal Line Marker"),  // cn:水平线标记
        QPen(Qt::red, 1, Qt::DashLine),
        QwtPlotMarker::HLine
    );
    marker->attach(plot);
    return marker;
}

/**
 * @brief 创建垂直线标记图元项
 * @param plot 关联的QwtPlot
 * @param pos 坐标位置
 * @return 创建的QwtPlotItem指针
 */
QwtPlotItem* createVLineMarkerPlotItem(QwtPlot* plot, const QPointF& pos)
{
    QwtPlotMarker* marker = createMarkerPlotItem(
        pos,
        QObject::tr("Vertical Line Marker"),  // cn:垂直直线标记
        QPen(Qt::red, 1, Qt::DashLine),
        QwtPlotMarker::VLine
    );
    marker->attach(plot);
    return marker;
}

/**
 * @brief 创建十字线标记图元项
 * @param plot 关联的QwtPlot
 * @param pos 坐标位置
 * @return 创建的QwtPlotItem指针
 */
QwtPlotItem* createCrossLineMarkerPlotItem(QwtPlot* plot, const QPointF& pos)
{
    QwtPlotMarker* marker = createMarkerPlotItem(
        pos,
        QObject::tr("Cross Line Marker"),  // cn:十字线标记
        QPen(Qt::red, 1, Qt::DashLine),
        QwtPlotMarker::Cross
    );
    marker->attach(plot);
    return marker;
}

/**
 * @brief 创建垂直数据探针图元项
 * @param plot 关联的QwtPlot
 * @param pos 坐标位置
 * @return 创建的QwtPlotItem指针
 */
QwtPlotItem* createVerticalDataProbePlotItem(QwtPlot* plot, const QPointF& pos)
{
    DADataProbeMarker* probe = new DADataProbeMarker(DADataProbeMarker::VerticalProbe);
    probe->setXValue(pos.x());
    probe->setProbeColor(Qt::blue);
    probe->attach(plot);
    probe->captureData(true);
    return probe;
}

/**
 * @brief 创建水平数据探针图元项
 * @param plot 关联的QwtPlot
 * @param pos 坐标位置
 * @return 创建的QwtPlotItem指针
 */
QwtPlotItem* createHorizontalDataProbePlotItem(QwtPlot* plot, const QPointF& pos)
{
    DADataProbeMarker* probe = new DADataProbeMarker(DADataProbeMarker::HorizontalProbe);
    probe->setYValue(pos.y());
    probe->setProbeColor(Qt::darkGreen);
    probe->attach(plot);
    probe->captureData(true);
    return probe;
}

}  // namespace DA