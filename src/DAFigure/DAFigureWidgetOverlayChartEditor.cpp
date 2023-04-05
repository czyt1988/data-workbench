#include "DAFigureWidgetOverlayChartEditor.h"
#include "DAChartWidget.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHash>
#include <QCoreApplication>
#include <QDebug>
#define DAFigureWidgetOverlayChartEditor_DebugPrint 0
namespace DA
{

class DAFigureWidgetOverlayChartEditorPrivate
{
    DA_IMPL_PUBLIC(DAFigureWidgetOverlayChartEditor)
public:
    QPoint _lastMousePressPos;  ///< 记录最后一次窗口移动的坐标
    QBrush _contorlPointBrush;  ///< 绘制chart2d在编辑模式下控制点的画刷
    QPen _borderPen;            ///< 绘制chart2d在编辑模式下的画笔
    bool _isStartResize;        ///< 标定开始进行缩放
    QWidget* _activeWidget;     ///标定当前激活的窗口，如果没有就为nullptr
    QRect _oldGeometry;         ///< 保存旧的窗口位置，用于redo/undo
    QSize _controlPointSize;    ///< 控制点大小
    DAFigureWidgetOverlayChartEditor::ControlType _controlType;  ///< 记录当前缩放窗口的位置情况

    bool _figOldMouseTracking;  /// 记录原来fig是否设置了mousetrack
    bool _figOldHasHoverAttr;   ///<记录原来的figure是否含有Hover属性
    bool _showPrecentText;      ///< 显示占比文字
    DAFigureWidgetOverlayChartEditorPrivate(DAFigureWidgetOverlayChartEditor* p)
        : q_ptr(p)
        , _lastMousePressPos(0, 0)
        , _contorlPointBrush(Qt::blue)
        , _borderPen(Qt::blue)
        , _isStartResize(false)
        , _activeWidget(nullptr)
        , _controlPointSize(8, 8)
        , _figOldMouseTracking(false)
        , _figOldHasHoverAttr(false)
        , _showPrecentText(true)
    {
    }
};

//===================================================
// DAFigureWidgetChartRubberbandEditOverlay
//===================================================

DAFigureWidgetOverlayChartEditor::DAFigureWidgetOverlayChartEditor(DAFigureWidget* fig)
    : DAFigureWidgetOverlay(fig), d_ptr(new DAFigureWidgetOverlayChartEditorPrivate(this))
{
    // setAttribute( Qt::WA_TransparentForMouseEvents,false);
    setFocusPolicy(Qt::ClickFocus);
    //由于QwtWidgetOverlay是对鼠标隐藏的，因此不能直接使用mouseEvent，直接捕获DAFigureWidget的event
    // QwtWidgetOverlay已经install了，因此无需再fig->installEventFilter(this);
    d_ptr->_figOldMouseTracking = fig->hasMouseTracking();
    if (!d_ptr->_figOldMouseTracking) {
        fig->setMouseTracking(true);
    }
    d_ptr->_figOldHasHoverAttr = fig->testAttribute(Qt::WA_Hover);
    if (!d_ptr->_figOldHasHoverAttr) {
        fig->setAttribute(Qt::WA_Hover, true);
    }
    DAChartWidget* curChart = figure()->getCurrentChart();
    if (curChart) {
        setActiveWidget(curChart);
    } else {
        selectNextChart();
        if (!getCurrentActiveWidget()) {
            selectNextWidget();
        }
    }
}

DAFigureWidgetOverlayChartEditor::~DAFigureWidgetOverlayChartEditor()
{
    //如果fig原来没有mousetrack，设置回去
    if (!d_ptr->_figOldMouseTracking) {
        figure()->setMouseTracking(false);
    }
    if (!d_ptr->_figOldHasHoverAttr) {
        figure()->setAttribute(Qt::WA_Hover, false);
    }
}

void DAFigureWidgetOverlayChartEditor::drawChartEditMode(QPainter* painter, const QRect& chartRect) const
{
    painter->setBrush(Qt::NoBrush);
    painter->setPen(d_ptr->_borderPen);
    QRect edgetRect = chartRect.adjusted(-1, -1, 1, 1);

    //绘制矩形边框
    painter->drawRect(edgetRect);
    //绘制边框到figure四周
    QPen linePen(d_ptr->_borderPen);

    linePen.setStyle(Qt::DotLine);
    painter->setPen(linePen);
    QPoint center = chartRect.center();

    painter->drawLine(center.x(), 0, center.x(), chartRect.top());            // top
    painter->drawLine(center.x(), chartRect.bottom(), center.x(), height());  // bottom
    painter->drawLine(0, center.y(), chartRect.left(), center.y());           // left
    painter->drawLine(chartRect.right(), center.y(), width(), center.y());    // right
    //绘制顶部数据
    QRectF percent  = DAFigureContainer::calcRectPosPercentByOtherRect(rect(), chartRect);
    QFontMetrics fm = painter->fontMetrics();
    // top text
    QString percentText = QString::number(percent.y() * 100, 'g', 2) + "%";
    QRectF textRect     = fm.boundingRect(percentText);
    textRect.moveTopLeft(QPoint(center.x(), 0));
    painter->drawText(textRect, Qt::AlignCenter, percentText);
    // left
    percentText = QString::number(percent.x() * 100, 'g', 2) + "%";
    textRect    = fm.boundingRect(percentText);
    textRect.moveBottomLeft(QPoint(0, center.y()));
    painter->drawText(textRect, Qt::AlignCenter, percentText);

    //    painter->drawText(QPointF(0, chartRect.y()), QString::number(percent.x(), 'g', 2));
    //绘制四个角落
    painter->setPen(Qt::NoPen);
    painter->setBrush(d_ptr->_contorlPointBrush);
    QRect connerRect(0, 0, d_ptr->_controlPointSize.width(), d_ptr->_controlPointSize.height());
    QPoint offset = QPoint(d_ptr->_controlPointSize.width() / 2, d_ptr->_controlPointSize.height() / 2);
    connerRect.moveTo(edgetRect.topLeft() - offset);
    painter->drawRect(connerRect);
    connerRect.moveTo(edgetRect.topRight() - offset);
    painter->drawRect(connerRect);
    connerRect.moveTo(edgetRect.bottomLeft() - offset);
    painter->drawRect(connerRect);
    connerRect.moveTo(edgetRect.bottomRight() - offset);
    painter->drawRect(connerRect);
}

///
/// \brief 根据范围获取图标
/// \param rr 范围
/// \return 图标
///
Qt::CursorShape DAFigureWidgetOverlayChartEditor::controlTypeToCursor(DAFigureWidgetOverlayChartEditor::ControlType rr)
{
    switch (rr) {
    case ControlLineTop:
    case ControlLineBottom:
        return (Qt::SizeVerCursor);

    case ControlLineLeft:
    case ControlLineRight:
        return (Qt::SizeHorCursor);

    case ControlPointTopLeft:
    case ControlPointBottomRight:
        return (Qt::SizeFDiagCursor);

    case ControlPointTopRight:
    case ControlPointBottomLeft:
        return (Qt::SizeBDiagCursor);

    case Inner:
        return (Qt::SizeAllCursor);

    default:
        break;
    }
    return (Qt::ArrowCursor);
}

///
/// \brief 根据点和矩形的关系，返回图标的样式
/// \param pos 点
/// \param region 矩形区域
/// \param err 允许误差
/// \return
///
DAFigureWidgetOverlayChartEditor::ControlType DAFigureWidgetOverlayChartEditor::getPositionControlType(const QPoint& pos,
                                                                                                       const QRect& region,
                                                                                                       int err)
{
    if (!region.adjusted(-err, -err, err, err).contains(pos)) {
        return (OutSide);
    }
    if (pos.x() < (region.left() + err)) {
        if (pos.y() < region.top() + err) {
            return (ControlPointTopLeft);
        } else if (pos.y() > region.bottom() - err) {
            return (ControlPointBottomLeft);
        }
        return (ControlLineLeft);
    } else if (pos.x() > (region.right() - err)) {
        if (pos.y() < region.top() + err) {
            return (ControlPointTopRight);
        } else if (pos.y() > region.bottom() - err) {
            return (ControlPointBottomRight);
        }
        return (ControlLineRight);
    } else if (pos.y() < (region.top() + err)) {
        if (pos.x() < region.left() + err) {
            return (ControlPointTopLeft);
        } else if (pos.x() > region.right() - err) {
            return (ControlPointTopRight);
        }
        return (ControlLineTop);
    } else if (pos.y() > region.bottom() - err) {
        if (pos.x() < region.left() + err) {
            return (ControlPointBottomLeft);
        } else if (pos.x() > region.right() - err) {
            return (ControlPointBottomRight);
        }
        return (ControlLineBottom);
    }
    return (Inner);
}

///
/// \brief 判断点是否在矩形区域的边缘
/// \param pos 点
/// \param region 矩形区域
/// \param err 允许误差
/// \return 如果符合边缘条件，返回true
///
bool DAFigureWidgetOverlayChartEditor::isPointInRectEdget(const QPoint& pos, const QRect& region, int err)
{
    if (!region.adjusted(-err, -err, err, err).contains(pos)) {
        return (false);
    }
    if ((pos.x() < (region.left() - err)) && (pos.x() < (region.left() + err))) {
        return (true);
    } else if ((pos.x() > (region.right() - err)) && (pos.x() < (region.right() + err))) {
        return (true);
    } else if ((pos.y() > (region.top() - err)) && (pos.y() < (region.top() + err))) {
        return (true);
    } else if ((pos.y() > region.bottom() - err) && (pos.y() < region.bottom() + err)) {
        return (true);
    }
    return (false);
}

/**
 * @brief 选择下一个窗口作为激活窗体
 * @param forward
 */
void DAFigureWidgetOverlayChartEditor::selectNextWidget(bool forward)
{
    QList< QWidget* > ws = figure()->getWidgetList();
    if (ws.isEmpty()) {
        setActiveWidget(nullptr);
        return;
    }
    if (d_ptr->_activeWidget) {
        //说明有窗口，找到这个窗口的下一个
        auto ite = std::find(ws.begin(), ws.end(), d_ptr->_activeWidget);
        if (ite != ws.end()) {
            //说明找到了
            if (forward) {
                //向前寻找
                ++ite;
                if (ite == ws.end()) {
                    ite = ws.begin();  //找到最后就把最前的给它
                }
            } else {
                //向后寻找
                if (ite != ws.begin()) {
                    --ite;
                } else {
                    //如果ite是最前，就把最后一个给予
                    ite = ws.end() - 1;
                }
            }
        }
        setActiveWidget(*ite);
    } else {
        //原来就没有激活窗口，给第一个
        setActiveWidget(ws.first());
    }
}

/**
 * @brief 选择下一个绘图作为激活窗体
 * @param forward
 */
void DAFigureWidgetOverlayChartEditor::selectNextChart(bool forward)
{
    QList< DAChartWidget* > ws = figure()->getCharts();
    if (ws.isEmpty()) {
        setActiveWidget(nullptr);
        return;
    }
    if (d_ptr->_activeWidget) {
        //说明有窗口，找到这个窗口的下一个
        auto ite = std::find(ws.begin(), ws.end(), d_ptr->_activeWidget);
        if (ite != ws.end()) {
            //说明找到了
            if (forward) {
                //向前寻找
                ++ite;
                if (ite == ws.end()) {
                    ite = ws.begin();  //找到最后就把最前的给它
                }
            } else {
                //向后寻找
                if (ite != ws.begin()) {
                    --ite;
                } else {
                    //如果ite是最前，就把最后一个给予
                    ite = ws.end() - 1;
                }
            }
        }
        setActiveWidget(*ite);
    } else {
        //原来就没有激活窗口，给第一个
        setActiveWidget(ws.first());
    }
}

/**
 * @brief 获取当前激活的窗体
 * @return
 */
QWidget* DAFigureWidgetOverlayChartEditor::getCurrentActiveWidget() const
{
    return d_ptr->_activeWidget;
}

/**
 * @brief 显示占比数值
 * @param on
 */
void DAFigureWidgetOverlayChartEditor::showPercentText(bool on)
{
    d_ptr->_showPrecentText = on;
    updateOverlay();
}

/**
 * @brief 设置边框的画笔
 * @param p
 */
void DAFigureWidgetOverlayChartEditor::setBorderPen(const QPen& p)
{
    d_ptr->_borderPen = p;
}

/**
 * @brief 边框的画笔
 * @param p
 */
QPen DAFigureWidgetOverlayChartEditor::getBorderPen() const
{
    return d_ptr->_borderPen;
}

/**
 * @brief 设置控制点的填充
 * @param b
 */
void DAFigureWidgetOverlayChartEditor::setControlPointBrush(const QBrush& b)
{
    d_ptr->_contorlPointBrush = b;
}

/**
 * @brief 控制点的填充
 * @param b
 */
QBrush DAFigureWidgetOverlayChartEditor::getControlPointBrush() const
{
    return d_ptr->_contorlPointBrush;
}

void DAFigureWidgetOverlayChartEditor::drawOverlay(QPainter* p) const
{
    if (d_ptr->_activeWidget) {
        //对于激活的窗口，绘制到四周的距离提示线
        p->save();
        drawChartEditMode(p, d_ptr->_activeWidget->frameGeometry());
        p->restore();
    }
}

QRegion DAFigureWidgetOverlayChartEditor::maskHint() const
{
    return (figure()->rect());
}

/**
 * @brief DAFigureWidgetChartRubberbandEditOverlay::eventFilter
 *
 * 注意，鼠标移动事件在setMouseTracking(true)后，button永远是NoButton,需要配合press事件才能判断
 * @param obj
 * @param event
 * @return
 */
bool DAFigureWidgetOverlayChartEditor::eventFilter(QObject* obj, QEvent* event)
{
    if (d_ptr->_activeWidget) {
        if (obj == figure()) {
#if DAFigureWidgetOverlayChartEditor_DebugPrint
            qDebug() << "Overlay eventFilter=" << event->type();
#endif
            //捕获DAChartWidget和DAFigure都是无法捕获正常鼠标移动的事件
            switch (event->type()) {
            case QEvent::MouseButtonPress: {
                QMouseEvent* me = static_cast< QMouseEvent* >(event);
                return (onMousePressedEvent(me));
            }

            case QEvent::MouseButtonRelease: {
                QMouseEvent* me = static_cast< QMouseEvent* >(event);
                return (onMouseReleaseEvent(me));
            }

            case QEvent::KeyPress: {
                QKeyEvent* ke = static_cast< QKeyEvent* >(event);
                return (onKeyPressedEvent(ke));
            }

            case QEvent::MouseMove: {
                QMouseEvent* e = static_cast< QMouseEvent* >(event);
                return (onMouseMoveEvent(e));
            }

            case QEvent::HoverMove: {
                QHoverEvent* e = static_cast< QHoverEvent* >(event);
                return (onHoverMoveEvent(e));
            }
            default:
                break;
            }
        }
    }

    // QwtWidgetOverlay也继承了eventFilter
    return (QwtWidgetOverlay::eventFilter(obj, event));
}

bool DAFigureWidgetOverlayChartEditor::onMouseMoveEvent(QMouseEvent* me)
{
    if (d_ptr->_activeWidget) {
        if (d_ptr->_isStartResize) { }
    }
    return (true);  //托管所有的鼠标事件
}

bool DAFigureWidgetOverlayChartEditor::onMouseReleaseEvent(QMouseEvent* me)
{
    if (Qt::LeftButton == me->button()) {
        if (d_ptr->_isStartResize) {
            d_ptr->_isStartResize = false;
            if (d_ptr->_activeWidget) {
                QRect newGeometry = d_ptr->_activeWidget->geometry();
                emit widgetGeometryChanged(d_ptr->_activeWidget, d_ptr->_oldGeometry, newGeometry);
                return (true);  //这里把消息截取不传递下去
            }
        }
    }
    return (true);  //托管所有的鼠标事件
}

bool DAFigureWidgetOverlayChartEditor::onMousePressedEvent(QMouseEvent* me)
{
    if (Qt::LeftButton == me->button()) {
        //左键点击
        QList< QWidget* > orderws = figure()->getOrderedWidgetList();
        for (auto ite = orderws.begin(); ite != orderws.end(); ++ite) {
            //点击到了其他窗体
            if ((*ite)->geometry().contains(me->pos())) {
                setActiveWidget(*ite);
            }
        }
        ControlType ct = getPositionControlType(me->pos(), d_ptr->_activeWidget->frameGeometry(), 4);
        if (OutSide == ct) {
            //如果点击了外部，那么久尝试变更激活窗口
            QList< QWidget* > ws = figure()->getWidgetList();
            for (QWidget* w : qAsConst(ws)) {
                if (w->frameGeometry().contains(me->pos(), true)) {
                    if (d_ptr->_activeWidget != w) {
                        setActiveWidget(w);
                        updateOverlay();
                        return (true);  //这里把消息截取不传递下去
                    }
                }
            }
        } else {
            //点击了其他区域，就执行变换
            d_ptr->_oldGeometry       = d_ptr->_activeWidget->geometry();
            d_ptr->_lastMousePressPos = me->pos();
            d_ptr->_isStartResize     = true;
            d_ptr->_controlType       = ct;
            return (true);  //这里把消息截取不传递下去
        }

        //没有点击到任何的地方就
    }
    return (true);  //托管所有的鼠标事件
}

bool DAFigureWidgetOverlayChartEditor::onHoverMoveEvent(QHoverEvent* me)
{
    if (d_ptr->_activeWidget) {
        if (d_ptr->_isStartResize) {
            //开始resize（鼠标按下左键后触发为true）
            //! 注意，不要在onMouseMoveEvent进行处理，因为鼠标移动到子窗体后，
            //! onMouseMoveEvent不会触发，但onHoverMoveEvent还会继续触发
#if DAFigureWidgetOverlayChartEditor_DebugPrint
            qDebug() << "DAFigureWidgetChartRubberbandEditOverlay::onHoverMoveEvent(" << me->pos() << ")"
                     << "\n   lastMousePressPos=" << d_ptr->_lastMousePressPos
                     << "\n   oldGeometry=" << d_ptr->_oldGeometry << "\n   controlType=" << d_ptr->_controlType;
#endif
            QRect geoRect = d_ptr->_oldGeometry;
            switch (d_ptr->_controlType) {
            case ControlLineTop: {
                QPoint offset = me->pos() - d_ptr->_lastMousePressPos;
                geoRect.adjust(0, offset.y(), 0, 0);
                d_ptr->_activeWidget->setGeometry(geoRect);
                break;
            }

            case ControlLineBottom: {
                int resultY = me->pos().y();
                geoRect.adjust(0, 0, 0, resultY - geoRect.bottom());
                d_ptr->_activeWidget->setGeometry(geoRect);
                break;
            }

            case ControlLineLeft: {
                int resultX = me->pos().x();
                geoRect.adjust(resultX - geoRect.left(), 0, 0, 0);
                d_ptr->_activeWidget->setGeometry(geoRect);
                break;
            }

            case ControlLineRight: {
                int resultX = me->pos().x();
                geoRect.adjust(0, 0, resultX - geoRect.right(), 0);
                d_ptr->_activeWidget->setGeometry(geoRect);
                break;
            }

            case ControlPointTopLeft: {
                geoRect.adjust(me->pos().x() - geoRect.left(), me->pos().y() - geoRect.top(), 0, 0);
                d_ptr->_activeWidget->setGeometry(geoRect);
                break;
            }

            case ControlPointTopRight: {
                geoRect.adjust(0, me->pos().y() - geoRect.top(), me->pos().x() - geoRect.right(), 0);
                d_ptr->_activeWidget->setGeometry(geoRect);
                break;
            }

            case ControlPointBottomLeft: {
                geoRect.adjust(me->pos().x() - geoRect.left(), 0, 0, me->pos().y() - geoRect.bottom());
                d_ptr->_activeWidget->setGeometry(geoRect);
                break;
            }

            case ControlPointBottomRight: {
                geoRect.adjust(0, 0, me->pos().x() - geoRect.right(), me->pos().y() - geoRect.bottom());
                d_ptr->_activeWidget->setGeometry(geoRect);
                break;
            }

            case Inner: {
                QPoint offset = me->pos() - d_ptr->_lastMousePressPos;
                geoRect.moveTo(d_ptr->_oldGeometry.topLeft() + offset);
                d_ptr->_activeWidget->setGeometry(geoRect);
                break;
            }

            default:
                return (false);
            }
            updateOverlay();
            return (true);  //这里把消息截取不传递下去
        } else {
            ControlType ct = getPositionControlType(me->pos(), d_ptr->_activeWidget->frameGeometry(), 4);
            if (d_ptr->_controlType != ct) {
                //说明控制点变更
                Qt::CursorShape cur = controlTypeToCursor(ct);
                figure()->setCursor(cur);
                d_ptr->_controlType = ct;
            }
        }
    }
    return (true);  //托管所有的鼠标事件
}

bool DAFigureWidgetOverlayChartEditor::onKeyPressedEvent(QKeyEvent* ke)
{
#if DAFigureWidgetOverlayChartEditor_DebugPrint
    qDebug() << "DAFigureWidgetChartRubberbandEditOverlay::onKeyPressedEvent:key=" << Qt::Key(ke->key());
#endif
    switch (ke->key()) {
    case Qt::Key_Return: {
        selectNextWidget(true);
    } break;

    case Qt::Key_Up:
    case Qt::Key_Left:
        selectNextWidget(true);
        break;

    case Qt::Key_Right:
    case Qt::Key_Down:
        selectNextWidget(false);
        break;

    default:
        return (false);
    }
    return (true);  //这里把消息截取不传递下去
}

/**
 * @brief 控制点尺寸
 * @default 8*8
 * @return
 */
QSize DAFigureWidgetOverlayChartEditor::getControlPointSize() const
{
    return d_ptr->_controlPointSize;
}

/**
 * @brief 设置控制点尺寸
 * @param c
 */
void DAFigureWidgetOverlayChartEditor::setControlPointSize(const QSize& c)
{
    d_ptr->_controlPointSize = c;
}

/**
 * @brief 改变激活窗口
 * @param w 如果w和当前的activeWidget一样，不做任何动作
 * @note 此函数会发射信号activeWidgetChanged
 * @sa activeWidgetChanged
 */
void DAFigureWidgetOverlayChartEditor::setActiveWidget(QWidget* w)
{
    if (w == d_ptr->_activeWidget) {
        //避免嵌套
        return;
    }
    QWidget* oldact      = d_ptr->_activeWidget;
    d_ptr->_activeWidget = w;
    updateOverlay();
    emit activeWidgetChanged(oldact, w);
}

}
