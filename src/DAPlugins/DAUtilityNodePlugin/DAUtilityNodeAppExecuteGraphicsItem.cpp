#include "DAUtilityNodeAppExecuteGraphicsItem.h"
#include <QSvgRenderer>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include "DAAbstractNodeGraphicsItem.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DANodeGraphicsScene.h"
#include "DACommandsForWorkFlowNodeGraphics.h"
#include "DAUtilityNodeFactory.h"
#include "DACoreInterface.h"
#define RES_APP_SVG ":/plugin-node-icon/icon/application.svg"
namespace DA
{
DAUtilityNodeAppExecuteGraphicsItem::DAUtilityNodeAppExecuteGraphicsItem(DAUtilityNodeAppExecute* n, QGraphicsItem* p)
    : DAStandardNodeSvgGraphicsItem(n, p)
{
    static QSvgRenderer s_app_shared_rende(QString(RES_APP_SVG));
    setSharedRenderer(&s_app_shared_rende);
    setBodySize(QSizeF(46, 46));
    setLinkPointShowType(DAAbstractNodeGraphicsItem::LinkPointShowOnHover);
    setBodyMinimumSize(QSize(20, 20));
    setText(n->getNodeName());
    setEnableResize(false);
#if DAUTILITYNODEPLUGIN_CHECK_PAINT
    setCheckShow(false);
#endif
}

DAUtilityNodeAppExecuteGraphicsItem::~DAUtilityNodeAppExecuteGraphicsItem()
{
}

std::shared_ptr< DAUtilityNodeAppExecute > DAUtilityNodeAppExecuteGraphicsItem::executeNode()
{
    return std::static_pointer_cast< DAUtilityNodeAppExecute >(node());
}
#if DAUTILITYNODEPLUGIN_CHECK_PAINT
void DAUtilityNodeAppExecuteGraphicsItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    DAStandardNodeSvgGraphicsItem::paintBody(painter, option, widget, bodyRect);
    static QSvgRenderer s_svg_question(QString(":/plugin-node-icon/icon/question.svg"));
    static QSvgRenderer s_svg_ok(QString(":/plugin-node-icon/icon/ok.svg"));
    QRectF checkRegin = bodyRect.adjusted(bodyRect.width() / 2, bodyRect.height() / 2, 0, 0);
    if (mIsCheck) {
        s_svg_ok.render(painter, checkRegin);
    } else {
        s_svg_question.render(painter, checkRegin);
    }
}

void DAUtilityNodeAppExecuteGraphicsItem::setCheckShow(bool on)
{
    mIsCheck = on;
    update();
}
#endif
/**
 * @brief 双击时单独执行节点，不用通过DAWorkflowExecute调度
 * @param event
 */
void DAUtilityNodeAppExecuteGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{

    if (nullptr == e) {
        return;
    }
    if (e->button() != Qt::LeftButton) {
        DAStandardNodeSvgGraphicsItem::mouseDoubleClickEvent(e);
        return;
    }
    if (!getBodyRect().contains(e->pos())) {
        DAStandardNodeSvgGraphicsItem::mouseDoubleClickEvent(e);
        return;
    }
    std::shared_ptr< DAUtilityNodeAppExecute > n = executeNode();
    n->exec();
#if DAUTILITYNODEPLUGIN_CHECK_PAINT
    setCheckShow(true);
#endif
}

}  // end DA
