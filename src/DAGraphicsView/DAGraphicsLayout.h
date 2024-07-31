#ifndef DAGRAPHICSLAYOUT_H
#define DAGRAPHICSLAYOUT_H
#include <QObject>
#include <QGraphicsItem>
#include "DAGraphicsViewGlobal.h"
namespace DA
{

/**
 * @brief 场景图层
 *
 * 图层可以对item进行批量操作，这里的图层更像visio的图层而不是ps的图层，ps的图层决定了z坐标，vision的图层不决定z坐标
 */
class DAGRAPHICSVIEW_API DAGraphicsLayout : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsLayout)
public:
    DAGraphicsLayout(QObject* par = nullptr);
    virtual ~DAGraphicsLayout();
    // 图层名字
    QString getName() const;
    void setName(const QString v);
    // 图层添加item
    void addItem(QGraphicsItem* v);
};
}

#endif  // DAGRAPHICSLAYOUT_H
