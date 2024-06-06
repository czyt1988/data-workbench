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
    enum PasteMode
    {
        PaseteRangeCenterToViewCenter,  ///< 粘贴内容的中心到视图的中心
        PaseteRangeCenterToCursor       ///< 粘贴内容的中心到光标点击处
    };

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
    void copySelectItems();
    // 复制到剪切板
    void copyItems(const QList< DAGraphicsItem* >& its, bool isCopy = true);
    // 复制当前选中的items
    void cutSelectItems();
    // 粘贴
    void paste(PasteMode mode = PaseteRangeCenterToViewCenter);
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
signals:

	/**
	 * @brief 节点删除
	 * @param item 节点
	 */
	void nodeItemDeleted(const QList< QGraphicsItem* >& items);

protected:
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void keyReleaseEvent(QKeyEvent* event) override;
    // 粘贴到视图中心
    void pasteToViewCenter();
};

}  // namespace DA
#endif  // GNODEGRAPHICSVIEW_H
