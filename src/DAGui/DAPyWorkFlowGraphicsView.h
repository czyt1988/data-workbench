#ifndef DAPyWorkFlowGraphicsView_H
#define DAPyWorkFlowGraphicsView_H
#include <QtCore/qglobal.h>
#include <QPointer>
#include <QUndoStack>
#include "DAGuiAPI.h"
#include "DAGraphicsView.h"
#include "DAGraphicsItem.h"
#include "DAPyNodeFactory.h"

class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;

namespace DA
{
class DAPyWorkFlowGraphicsScene;
class DAPyWorkFlow;
class DAPyNodeGraphicsItem;
/**
 * @brief 用于节点显示的GraphicsView
 */
class DAGUI_API DAPyWorkFlowGraphicsView : public DAGraphicsView
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPyWorkFlowGraphicsView)
public:
	DAPyWorkFlowGraphicsView(QWidget* parent = 0);
	DAPyWorkFlowGraphicsView(QGraphicsScene* scene, QWidget* parent = 0);
	~DAPyWorkFlowGraphicsView();
	void setWorkFlow(DAPyWorkFlow* wf);
	DAPyWorkFlow* getWorkflow();
	// 激活UndoStack
	void setUndoStackActive();
	QUndoStack* getUndoStack();
	DAPyWorkFlowGraphicsScene* getWorkFlowGraphicsScene();
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
	DAPyNodeGraphicsItem* createNode(const DAPyNodeMetaData& md, const QPoint& pos);
	DAPyNodeGraphicsItem* createNode_(const DAPyNodeMetaData& md, const QPoint& pos);
	// 通过node元对象创建工作流节点（直接传递元数据，避免QJsonObject中间转换导致数据丢失）
	DAPyNodeGraphicsItem* createNode(const DAPyNodeMetaData& md, const QPointF& pos);
	DAPyNodeGraphicsItem* createNode_(const DAPyNodeMetaData& md, const QPointF& pos);

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
#endif  // DAPYWORKFLOWGRAPHICSVIEW_H
