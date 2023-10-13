#include "DANodeGraphicsSceneEventListener.h"
#include "DAAbstractNodeFactory.h"
#include "DANodeGraphicsScene.h"
namespace DA
{
DANodeGraphicsSceneEventListener::DANodeGraphicsSceneEventListener(DAAbstractNodeFactory* fac) : QObject(fac)
{
}

DANodeGraphicsSceneEventListener::~DANodeGraphicsSceneEventListener()
{
}

/**
 * @brief 获取对应的工厂
 * @return
 */
DAAbstractNodeFactory* DANodeGraphicsSceneEventListener::getFactory() const
{
    return qobject_cast< DAAbstractNodeFactory* >(parent());
}

/**
 * @brief 获取监听的场景
 * @note 此函数要在sceneAddedFactory触发后调用才有效
 * @return 有可能返回nullptr
 */
DANodeGraphicsScene* DANodeGraphicsSceneEventListener::getScene() const
{
    return mScene.data();
}

/**
 * @brief 场景首次添加的虚函数
 *
 * @note 如果重载，必须调用此函数DANodeGraphicsSceneEventListener::sceneAddedFactory(sc);，
 * 否则@sa getScene 函数无法生效
 *
 * @param sc
 */
void DANodeGraphicsSceneEventListener::factoryAddedToScene(DANodeGraphicsScene* sc)
{
    mScene = sc;
}

/**
 * @brief 针对DANodeGraphicsScene的mousePressEvent
 * @param sc
 * @param mouseEvent
 */
void DANodeGraphicsSceneEventListener::mousePressEvent(DANodeGraphicsScene* sc, QGraphicsSceneMouseEvent* mouseEvent)
{
    Q_UNUSED(sc);
    Q_UNUSED(mouseEvent);
}

/**
 * @brief 针对DANodeGraphicsScene的mouseMoveEvent
 * @param sc
 * @param mouseEvent
 */
void DANodeGraphicsSceneEventListener::mouseMoveEvent(DANodeGraphicsScene* sc, QGraphicsSceneMouseEvent* mouseEvent)
{
    Q_UNUSED(sc);
    Q_UNUSED(mouseEvent);
}

/**
 * @brief 针对DANodeGraphicsScene的mouseReleaseEvent
 * @param sc
 * @param mouseEvent
 */
void DANodeGraphicsSceneEventListener::mouseReleaseEvent(DANodeGraphicsScene* sc, QGraphicsSceneMouseEvent* mouseEvent)
{
    Q_UNUSED(sc);
    Q_UNUSED(mouseEvent);
}

}
