#include "DAWorkFlowGraphicsView.h"
#include <QDragEnterEvent>
#include <QDebug>
#include <QKeyEvent>
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

void DAWorkFlowGraphicsView::keyPressEvent(QKeyEvent* event)
{
	// qDebug() << "DAWorkFlowGraphicsView::keyPressEvent isAccepted =" << event->isAccepted();
	if (event->modifiers().testFlag(Qt::ControlModifier)) {
		// Ctrl键

	} else {
		switch (event->key()) {
		case Qt::Key_Delete:  // 删除操作
		{
			DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
			if (sc) {
				if (sc->removeSelectedItems_() > 0) {
					// 说明成功删除了
					event->accept();
					return;
				}
			}
		} break;
		case Qt::Key_Escape:  // 取消操作
		{
			DANodeGraphicsScene* sc = getWorkFlowGraphicsScene();
			if (sc) {
				if (sc->isStartLink()) {
					sc->cancelLink();
					event->accept();
				} else {
					// 不在连线状态按下esc，就取消选择
					sc->clearSelection();
					event->accept();
				}
				return;
			}
		} break;
		default:
			break;
		}
	}
	// 向下传递
	DAGraphicsView::keyPressEvent(event);
}

/**
 * @brief 复制条目
 * @param its
 */
void DAWorkFlowGraphicsView::copyItems(const QList< DAGraphicsItem* > its)
{
	DAXmlHelper xml;
	QDomDocument doc;
	QDomElement rootEle = xml.makeClipBoardElement(its, &doc, true);

	//! 4.保存节点
}

}  // end DA
