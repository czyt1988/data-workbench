#include "DAChartPointerSelectorEditor.h"
#include "DAChartElementHitTester.h"
#include "da_qt5qt6_compat.hpp"
// Qt
#include <QMouseEvent>
// qwt
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_map.h"

namespace DA
{
class DAChartPointerSelectorEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartPointerSelectorEditor)
public:
    PrivateData(DAChartPointerSelectorEditor* p);
    // item失效检测（QwtPlotItem非QObject，只能用plot()->itemList包含性判断）
    bool isItemAlive(QwtPlotItem* item) const;

public:
    QwtPlotItem* mCurrentItem { nullptr };  ///< 当前选中的item（生命周期由plot管理）
    bool mIsDragging { false };             ///< 拖动态
    QPoint mPressCanvasPos;                 ///< 按下时canvas坐标
    DAChartElementHitTester::ItemGeometry mDragOldGeo;  ///< 拖动前几何
    DAChartElementHitTester::ItemGeometry mDragNewGeo;  ///< 拖动后几何
};

DAChartPointerSelectorEditor::PrivateData::PrivateData(DAChartPointerSelectorEditor* p) : q_ptr(p)
{
}

/**
 * @brief item 是否仍然 attach 在当前 plot 上
 *
 * QwtPlotItem 不是 QObject，无法用 QPointer 跟踪生命周期，
 * 用 plot 的 itemList 包含性判断（detach/删除后不在列表中）
 */
bool DAChartPointerSelectorEditor::PrivateData::isItemAlive(QwtPlotItem* item) const
{
    if (!item) {
        return false;
    }
    const QwtPlot* p = q_ptr->plot();
    return p && p->itemList().contains(item);
}

/**
 * @brief 构造函数
 * @param parent 关联的QwtPlot
 */
DAChartPointerSelectorEditor::DAChartPointerSelectorEditor(QwtPlot* parent)
    : DAAbstractChartEditor(parent), DA_PIMPL_CONSTRUCT
{
    // 基类eventFilter对Esc会调cancel()+finishedEdit结束编辑，
    // 指针模式是持久会话，Esc由overlay统一处理（退出整个指针模式），此处屏蔽
    setBlockKeys(QList< int >() << Qt::Key_Escape);
}

/**
 * @brief 析构函数
 */
DAChartPointerSelectorEditor::~DAChartPointerSelectorEditor()
{
}

/**
 * @brief 运行时类型标识（RTTIPointerSelector 在 DAAbstractChartEditor 中新增）
 * @return RTTI值
 */
int DAChartPointerSelectorEditor::rtti() const
{
    return DAAbstractChartEditor::RTTIPointerSelector;
}

/**
 * @brief 交出编辑产出的item
 *
 * 指针模式不产出item，恒返回nullptr
 * @return nullptr
 */
QwtPlotItem* DAChartPointerSelectorEditor::takeItem()
{
    return nullptr;
}

/**
 * @brief 获取当前选中的item
 * @return 选中的item，无选中返回nullptr
 */
QwtPlotItem* DAChartPointerSelectorEditor::currentItem() const
{
    return d_ptr->mCurrentItem;
}

/**
 * @brief 程序化设置选中item
 *
 * 不发选中信号，用于 overlay 在激活 plot 切换时同步状态
 * @param item 选中的item，nullptr表示清除选中
 */
void DAChartPointerSelectorEditor::setCurrentItem(QwtPlotItem* item)
{
    d_ptr->mCurrentItem = item;
}

/**
 * @brief 鼠标按下：item 命中测试与拖动启动
 *
 * 命中 item → 选中并发射 itemClicked；命中的是已选中可移动 item → 同时进入拖动态；
 * 未命中 → 发射 itemClicked(nullptr)，由 overlay 决定空白点击行为
 * @param e 鼠标事件（canvas坐标）
 * @return 恒返回true，指针模式独占canvas左键
 */
bool DAChartPointerSelectorEditor::mousePressEvent(const QMouseEvent* e)
{
    DA_D(d);
    if (e->button() != Qt::LeftButton) {
        return false;
    }
    const QPoint canvasPos = compat::eventPos(e);
    QwtPlotItem* hitItem   = DAChartElementHitTester::itemAt(plot(), canvasPos);
    if (!hitItem) {
        // 空白点击，由overlay处理（取消选中/切回chart设置）
        d->mCurrentItem = nullptr;
        Q_EMIT itemClicked(this, nullptr);
        return true;
    }
    const bool firstSelect  = (hitItem != d->mCurrentItem);
    d->mCurrentItem         = hitItem;
    Q_EMIT itemClicked(this, hitItem);
    // 按住已选中的可移动item，进入拖动
    if (!firstSelect && DAChartElementHitTester::isMovableItem(hitItem)) {
        d->mIsDragging     = true;
        d->mPressCanvasPos = canvasPos;
        d->mDragOldGeo     = DAChartElementHitTester::itemGeometry(hitItem);
        d->mDragNewGeo     = d->mDragOldGeo;
        Q_EMIT itemDragBegan();
    }
    return true;
}

/**
 * @brief 鼠标移动：拖动态下实时平移item
 * @param e 鼠标事件（canvas坐标）
 * @return 拖动态返回true
 */
bool DAChartPointerSelectorEditor::mouseMoveEvent(const QMouseEvent* e)
{
    DA_D(d);
    if (!d->mIsDragging || !d->mCurrentItem) {
        return false;
    }
    const QPoint canvasPos = compat::eventPos(e);
    const QPointF delta    = dataDelta(d->mPressCanvasPos, canvasPos);
    if (delta.manhattanLength() <= 0.0) {
        return true;
    }
    DAChartElementHitTester::moveItemBy(d->mCurrentItem, delta);
    d->mDragNewGeo = DAChartElementHitTester::itemGeometry(d->mCurrentItem);
    if (QwtPlot* p = plot()) {
        p->replot();
    }
    Q_EMIT itemGeometryChanged();
    return true;
}

/**
 * @brief 鼠标释放：结束拖动
 *
 * 位移为零时 oldGeo==newGeo，宿主据此跳过 undo 提交
 * @param e 鼠标事件
 * @return 处理返回true
 */
bool DAChartPointerSelectorEditor::mouseReleaseEvent(const QMouseEvent* e)
{
    DA_D(d);
    Q_UNUSED(e);
    if (!d->mIsDragging) {
        return false;
    }
    d->mIsDragging = false;
    d->mDragNewGeo = d->mCurrentItem ? DAChartElementHitTester::itemGeometry(d->mCurrentItem)
                                     : DAChartElementHitTester::ItemGeometry();
    Q_EMIT itemDragEnded(d->mCurrentItem, d->mDragOldGeo, d->mDragNewGeo);
    return true;
}

/**
 * @brief 计算两次canvas位置的位移（数据坐标）
 * @param fromCanvasPos 起始canvas坐标
 * @param toCanvasPos 当前canvas坐标
 * @return 数据坐标位移
 */
QPointF DAChartPointerSelectorEditor::dataDelta(const QPoint& fromCanvasPos, const QPoint& toCanvasPos) const
{
    const QwtPlot* gca = plot();
    if (!gca) {
        return QPointF();
    }
    // 拖动的item可能挂在非可见轴上，使用item自己的轴映射
    if (d_ptr->mCurrentItem) {
        const QwtScaleMap& xMap = gca->canvasMap(d_ptr->mCurrentItem->xAxis());
        const QwtScaleMap& yMap = gca->canvasMap(d_ptr->mCurrentItem->yAxis());
        return QPointF(xMap.invTransform(toCanvasPos.x()) - xMap.invTransform(fromCanvasPos.x()),
                       yMap.invTransform(toCanvasPos.y()) - yMap.invTransform(fromCanvasPos.y()));
    }
    const QwtScaleMap& xMap = gca->canvasMap(gca->visibleXAxisId());
    const QwtScaleMap& yMap = gca->canvasMap(gca->visibleYAxisId());
    return QPointF(xMap.invTransform(toCanvasPos.x()) - xMap.invTransform(fromCanvasPos.x()),
                   yMap.invTransform(toCanvasPos.y()) - yMap.invTransform(fromCanvasPos.y()));
}
}  // namespace DA
