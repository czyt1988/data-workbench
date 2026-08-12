#include "DAAbstractChartEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include "DAChartWidget.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param parent 关联的QwtPlot
 */
DAAbstractChartEditor::DAAbstractChartEditor(QwtPlot* parent) : QObject(parent), mIsEnable(false)
{
}

/**
 * @brief 析构函数
 */
DAAbstractChartEditor::~DAAbstractChartEditor()
{
}

/**
 * @brief 获取关联的QwtPlot（const版本）
 * @return 关联的QwtPlot指针
 */
const QwtPlot* DAAbstractChartEditor::plot() const
{
    return qobject_cast< const QwtPlot* >(parent());
}

/**
 * @brief 获取关联的QwtPlot
 * @return 关联的QwtPlot指针
 */
QwtPlot* DAAbstractChartEditor::plot()
{
    return qobject_cast< QwtPlot* >(parent());
}

/**
 * @brief 获取关联的DAChartWidget（const版本）
 * @return 关联的DAChartWidget指针
 */
const DAChartWidget* DAAbstractChartEditor::chart() const
{
    return qobject_cast< const DAChartWidget* >(plot());
}

/**
 * @brief 获取关联的DAChartWidget
 * @return 关联的DAChartWidget指针
 */
DAChartWidget* DAAbstractChartEditor::chart()
{
    return qobject_cast< DAChartWidget* >(plot());
}

/**
 * @brief 设置是否启用编辑器
 * @param on 是否启用
 */
void DAAbstractChartEditor::setEnabled(bool on)
{
    if (on == mIsEnable)
        return;

    QwtPlot* p = plot();
    if (p) {
        mIsEnable = on;

        if (p->canvas()) {
            if (on) {
                p->canvas()->installEventFilter(this);
            } else {
                p->canvas()->removeEventFilter(this);
            }
        }
    }
}

/**
 * @brief 判断编辑器是否启用
 * @return 如果启用返回true，否则返回false
 */
bool DAAbstractChartEditor::isEnabled() const
{
    return mIsEnable;
}

/**
 * @brief 取消编辑
 *
 * 默认按Esc取消,按Esc后的动作可以重载此函数
 * @return 返回true说明取消成功，返回false说明取消失败，取消成功不会继续响应，
 * 并发送@ref editorFinished(true)信号
 *
 * @note 默认取消成功后，会调用etEnable(false)，editor不在监听plot
 */
bool DAAbstractChartEditor::cancel()
{
    return true;
}

/**
 * @brief 设置拦截的按键列表
 * @param keys 按键列表
 */
void DAAbstractChartEditor::setBlockKeys(const QList< int >& keys)
{
    mBlockKeys = keys;
}

/**
 * @brief 获取拦截的按键列表
 * @return 按键列表
 */
const QList< int >& DAAbstractChartEditor::getBlockKeys() const
{
    return mBlockKeys;
}


/**
 * @brief 把plot点映射到canvas上
 * @param pos
 * @return
 */
QPoint DAAbstractChartEditor::mapPlotPosToCanvasPos(const QPoint& pos) const
{
    const QwtPlot* p      = plot();
    const QWidget* canvas = p->canvas();
    if (!canvas) {
        return pos;
    }
    QPoint gp = p->mapToGlobal(pos);
    return canvas->mapFromGlobal(gp);
}

/**
 * @brief 事件过滤器
 * @param object 监听的对象
 * @param event 事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractChartEditor::eventFilter(QObject* object, QEvent* event)
{
    QwtPlot* plot = qobject_cast< QwtPlot* >(parent());
    if (plot && (object == plot->canvas())) {
        switch (event->type()) {
            // 空格按下，鼠标事件不处理
        case QEvent::MouseButtonPress: {
            const QMouseEvent* mouseEvent = static_cast< QMouseEvent* >(event);
            if (mouseEvent) {
                return mousePressEvent(mouseEvent);
            } else {
                return false;
            }
            break;
        }
        case QEvent::MouseMove: {
            const QMouseEvent* mouseEvent = static_cast< QMouseEvent* >(event);
            if (mouseEvent) {
                return mouseMoveEvent(mouseEvent);
            } else {
                return false;
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            const QMouseEvent* mouseEvent = static_cast< QMouseEvent* >(event);
            if (mouseEvent) {
                return mouseReleaseEvent(mouseEvent);
            } else {
                return false;
            }
            break;
        }
        case QEvent::KeyPress: {
            const QKeyEvent* keyEvent = static_cast< QKeyEvent* >(event);
            if (keyEvent) {
                if (mBlockKeys.size() > 0 && mBlockKeys.contains(keyEvent->key())) {
                    return QObject::eventFilter(object, event);
                }
                if (Qt::Key_Escape == keyEvent->key()) {
                    if (cancel()) {
                        setEnabled(false);
                        Q_EMIT finishedEdit(true);
                    }
                }
                return keyPressEvent(keyEvent);
            }
            break;
        }
        case QEvent::KeyRelease: {
            const QKeyEvent* keyEvent = static_cast< QKeyEvent* >(event);
            if (keyEvent) {
                return keyReleaseEvent(keyEvent);
            }
            break;
        }
        default:
            break;
        }
        return false;
    }
    return QObject::eventFilter(object, event);
}

/**
 * @brief 鼠标按下事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractChartEditor::mousePressEvent(const QMouseEvent* e)
{
    Q_UNUSED(e);
    return false;
}

/**
 * @brief 鼠标移动事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractChartEditor::mouseMoveEvent(const QMouseEvent* e)
{
    Q_UNUSED(e);
    return false;
}

/**
 * @brief 鼠标释放事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractChartEditor::mouseReleaseEvent(const QMouseEvent* e)
{
    Q_UNUSED(e);
    return false;
}

/**
 * @brief 键盘按下事件处理
 * @param e 键盘事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractChartEditor::keyPressEvent(const QKeyEvent* e)
{
    Q_UNUSED(e);
    return false;
}

/**
 * @brief 键盘释放事件处理
 * @param e 键盘事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractChartEditor::keyReleaseEvent(const QKeyEvent* e)
{
    Q_UNUSED(e);
    return false;
}

/**
 * @brief 屏幕坐标转换为数据坐标
 * @param pos 屏幕坐标
 * @return 数据坐标
 */
QPointF DAAbstractChartEditor::invTransform(const QPointF& pos) const
{
    const QwtPlot* gca = plot();
    if (!gca) {
        return pos;
    }
    QwtScaleMap xMap = gca->canvasMap(gca->visibleXAxisId());
    QwtScaleMap yMap = gca->canvasMap(gca->visibleYAxisId());
    return QPointF(xMap.invTransform(pos.x()), yMap.invTransform(pos.y()));
}

/**
 * @brief 数据坐标转换为屏幕坐标
 * @param pos 数据坐标
 * @return 屏幕坐标
 */
QPointF DAAbstractChartEditor::transform(const QPointF& pos) const
{
    const QwtPlot* gca = plot();
    if (!gca) {
        return pos;
    }
    QwtScaleMap xMap = gca->canvasMap(gca->visibleXAxisId());
    QwtScaleMap yMap = gca->canvasMap(gca->visibleYAxisId());
    return QPointF(xMap.transform(pos.x()), yMap.transform(pos.y()));
}
/**
 * @brief 更新预览项
 *
 * 此函数用于编辑器更新预览项，预览项坐标为绘图坐标
 *
 * 在@ref DAAbstractChartEditor 中,此函数默认实现为空
 *
 * 在其它编辑器中，你可以根据鼠标移动事件来更新预览项，例如绘制矩形的情况，在确定第一个点后，
 * 鼠标的移动过程中就需要不停的更新矩形的预览，那么可以通过重写此函数来实现
 * @param points 预览项坐标(绘图坐标)
 */
void DAAbstractChartEditor::updatePreview(const QVector< QPointF >& points)
{
    Q_UNUSED(points);
}
}  // End Of Namespace DA
