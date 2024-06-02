#ifndef DAWORKFLOWGRAPHICSVIEW_H
#define DAWORKFLOWGRAPHICSVIEW_H
#include <QtCore/qglobal.h>
#include <QPointer>
#include <QUndoStack>
#include "DAGuiAPI.h"
#include "DAGraphicsView.h"
#include "DAGraphicsItem.h"
#include "DAAbstractNodeWidget.h"

namespace DA
{
class DAWorkFlowGraphicsScene;
class DADataWorkFlow;
/**
 * @brief 用于节点显示的GraphicsView
 */
class DAGUI_API DAWorkFlowGraphicsView : public DAGraphicsView
{
	Q_OBJECT
public:
	DAWorkFlowGraphicsView(QWidget* parent = 0);
	DAWorkFlowGraphicsView(QGraphicsScene* scene, QWidget* parent = 0);
	~DAWorkFlowGraphicsView();
	void setWorkFlow(DAWorkFlow* wf);
	DAWorkFlow* getWorkflow();
	// 激活UndoStack
	void setUndoStackActive();
	QUndoStack* getUndoStack();
	DAWorkFlowGraphicsScene* getWorkFlowGraphicsScene();

protected:
	void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
public slots:
	// 复制到剪切板
	void copyItems(QList< DAGraphicsItem* > its);
signals:

	/**
	 * @brief 节点删除
	 * @param item 节点
	 */
	void nodeItemDeleted(const QList< QGraphicsItem* >& items);
};
}  // namespace DA
#endif  // GNODEGRAPHICSVIEW_H
