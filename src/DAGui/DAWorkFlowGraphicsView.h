#ifndef DAWORKFLOWGRAPHICSVIEW_H
#define DAWORKFLOWGRAPHICSVIEW_H
#include <QtCore/qglobal.h>
#include <QPointer>
#include <QUndoStack>
#include "DAGuiAPI.h"
#include "DAGraphicsView.h"
#include "DAGraphicsItem.h"
#include "DAAbstractNodeWidget.h"

class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;

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
	DA_DECLARE_PRIVATE(DAWorkFlowGraphicsView)
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
	// 获取当前view视图下的scene中心
	QPointF getViewCenterMapToScene() const;
	// 把item移动到屏幕中心
	void moveItemToViewCenter(QGraphicsItem* item);
	// 复制当前选中的items
	bool copySelectItems();
	// 复制到剪切板
	void copyItems(const QList< DAGraphicsItem* >& its, bool isCopy = true);
	// 复制当前选中的items
	void cutSelectItems();
    // 粘贴,会发射pastedItems信号
	QList< QGraphicsItem* > paste();
	// 粘贴到视图中心
	void pasteToViewCenter();
	// 粘贴，同时偏移一个距离
	void pasteByOffset(const QPointF& offset);
	// 设置item的选中状态
	void setSelectionState(const QList< QGraphicsItem* >& items, bool isSelect);
	// 取消选中
	void clearSelection();
	// 全选
	void selectAll();
	// 计算item所包含的范围，这个范围存入xml中，以便让scene第一时间知道总体范围
	static QRectF calcItemsSceneRange(const QList< QGraphicsItem* >& its);
	//
	static QList< QGraphicsItem* > cast(const QList< DAGraphicsItem* >& its);
	// 所有item偏移一个距离
	void offsetItems(const QList< QGraphicsItem* >& its, const QPointF& offset);
	// 通过node元对象创建工作流节点
	DAAbstractNodeGraphicsItem* createNode(const DANodeMetaData& md, const QPoint& pos);
	DAAbstractNodeGraphicsItem* createNode_(const DANodeMetaData& md, const QPoint& pos);

signals:

	/**
	 * @brief 节点删除
	 * @param item 节点
	 */
	void nodeItemDeleted(const QList< QGraphicsItem* >& items);

    /**
     * @brief 粘贴触发的信号
     *
     * 此信号在粘贴完成时触发
     *
     * @param items 粘贴产生的新节点，此时节点都已经加入了场景中
     */
    void pastedItems(const QList< QGraphicsItem* >& items);

protected:
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void keyPressEvent(QKeyEvent* event) override;
	virtual void keyReleaseEvent(QKeyEvent* event) override;

	virtual void dragEnterEvent(QDragEnterEvent* event) override;
	virtual void dragMoveEvent(QDragMoveEvent* event) override;
	virtual void dragLeaveEvent(QDragLeaveEvent* event) override;
	virtual void dropEvent(QDropEvent* event) override;

private:
	void resetCursor();
};

}  // namespace DA
#endif  // GNODEGRAPHICSVIEW_H
