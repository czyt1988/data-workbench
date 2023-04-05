#ifndef DAUTILITYNODEAPPEXECUTEGRAPHICSITEM_H
#define DAUTILITYNODEAPPEXECUTEGRAPHICSITEM_H
#include "DAUtilityNodePluginAPI.h"
#include "DAUtilityNodeAppExecute.h"
#include "DAStandardNodeSvgGraphicsItem.h"
#define DAUTILITYNODEPLUGIN_CHECK_PAINT 1

class QPainter;
class QStyleOptionGraphicsItem;

namespace DA
{
class DAUTILITYNODEPLUGIN_API DAUtilityNodeAppExecuteGraphicsItem : public DAStandardNodeSvgGraphicsItem
{
    Q_OBJECT
public:
    DAUtilityNodeAppExecuteGraphicsItem(DAUtilityNodeAppExecute* n, QGraphicsItem* p = nullptr);
    ~DAUtilityNodeAppExecuteGraphicsItem();

    std::shared_ptr< DAUtilityNodeAppExecute > executeNode();

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);

#if DAUTILITYNODEPLUGIN_CHECK_PAINT
public:
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;
    void setCheckShow(bool on);

private:
    bool _isCheck;
#endif
};
}

#endif  // DAUTILITYNODEAPPEXECUTEGRAPHICSITEM_H
