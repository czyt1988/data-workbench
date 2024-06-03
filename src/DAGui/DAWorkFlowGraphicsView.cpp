#include "DAWorkFlowGraphicsView.h"
#include <QDragEnterEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include "DAAbstractNode.h"
#include "DANodeMetaData.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAWorkFlow.h"
#include "DAAbstractNodeGraphicsItem.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DAGraphicsItemGroup.h"
#include "DAXmlHelper.h"
#include <QDomDocument>
#include <QDomElement>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAWorkFlowGraphicsView
//===================================================
DAWorkFlowGraphicsView::DAWorkFlowGraphicsView(QWidget* parent) : DAGraphicsView(parent)
{
	setAcceptDrops(true);
	centerOn(0, 0);
	setRenderHint(QPainter::Antialiasing, true);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

DAWorkFlowGraphicsView::DAWorkFlowGraphicsView(QGraphicsScene* scene, QWidget* parent) : DAGraphicsView(scene, parent)
{
	setAcceptDrops(true);
	centerOn(0, 0);
	setRenderHint(QPainter::Antialiasing, true);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

DAWorkFlowGraphicsView::~DAWorkFlowGraphicsView()
{
	//    DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	//    if (sc) {
	//        sc->clear();
	//    }
	//    qDebug() << "destory DAWorkFlowGraphicsView";
}

/**
 * @brief DAWorkFlowGraphicsView不负责workflow的所有权
 * @param wf
 */
void DAWorkFlowGraphicsView::setWorkFlow(DAWorkFlow* wf)
{
	DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {
		return sc->setWorkFlow(wf);
	}
}

/**
 * @brief DAWorkFlowGraphicsView::workflow
 * @return
 */
DAWorkFlow* DAWorkFlowGraphicsView::getWorkflow()
{
	DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {
		return (sc->getWorkflow());
	}
	return nullptr;
}

void DAWorkFlowGraphicsView::setUndoStackActive()
{
	DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {
		sc->setUndoStackActive();
	}
}

QUndoStack* DAWorkFlowGraphicsView::getUndoStack()
{
	DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {

		return &(sc->undoStack());
	}
	return nullptr;
}

/**
 * @brief 获取DAWorkFlowGraphicsScene
 * @return
 */
DAWorkFlowGraphicsScene* DAWorkFlowGraphicsView::getWorkFlowGraphicsScene()
{
    return qobject_cast< DAWorkFlowGraphicsScene* >(scene());
}

}  // end DA
