#ifndef DANODEGRAPHICSSCENEEVENTLISTENER_H
#define DANODEGRAPHICSSCENEEVENTLISTENER_H
#include "DAWorkFlowGlobal.h"
#include <QObject>
#include <QPointer>
#include <QGraphicsSceneMouseEvent>
class QGraphicsSceneMouseEvent;
namespace DA
{
class DAAbstractNodeFactory;
class DANodeGraphicsScene;
/**
 * @brief 这个是DANodeGraphicsScene的事件监听类，factory如果有对DANodeGraphicsScene操作的需求，可以重写这个监听类
 *
 * DANodeGraphicsScene在调用@sa DANodeGraphicsScene::setWorkFlow时，会获取这个workflow的factory，
 * 并调用@sa DAAbstractNodeFactory::createNodeGraphicsSceneEventListener函数，获取每个工厂的listener，
 * 在对应的每个事件都会调用些listener，以实现事件的捕获
 */
class DAWORKFLOW_API DANodeGraphicsSceneEventListener : public QObject
{
    Q_OBJECT
public:
    DANodeGraphicsSceneEventListener(DAAbstractNodeFactory* fac);
    ~DANodeGraphicsSceneEventListener();
    //获取对应的工厂
    DAAbstractNodeFactory* getFactory() const;
    //获取监听的场景
    DANodeGraphicsScene* getScene() const;
    //场景首次添加的虚函数
    virtual void factoryAddedToScene(DANodeGraphicsScene* sc);
    //鼠标点击事件
    virtual void mousePressEvent(DANodeGraphicsScene* sc, QGraphicsSceneMouseEvent* mouseEvent);
    //鼠标移动事件
    virtual void mouseMoveEvent(DANodeGraphicsScene* sc, QGraphicsSceneMouseEvent* mouseEvent);
    //鼠标释放
    virtual void mouseReleaseEvent(DANodeGraphicsScene* sc, QGraphicsSceneMouseEvent* mouseEvent);

private:
    QPointer< DANodeGraphicsScene > mScene;
};
}

#endif  // DANODEGRAPHICSSCENEEVENTLISTENER_H
