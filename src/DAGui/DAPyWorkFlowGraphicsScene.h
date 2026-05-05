#ifndef DAPYWORKFLOWGRAPHICSSCENE_H
#define DAPYWORKFLOWGRAPHICSSCENE_H
#include <QGraphicsScene>
#include "DAGuiAPI.h"
#include <QHash>
#include <QUndoStack>
#include "DAPyWorkFlow.h"
#include "DAPyWorkFlowScene.h"

class QGraphicsSceneWheelEvent;
namespace DA
{
class DAGraphicsPixmapItem;
/**
 * @brief The DAPyWorkFlowGraphicsScene class
 */
	class DAGUI_API DAPyWorkFlowGraphicsScene : public DAPyWorkFlowScene
	{
		Q_OBJECT
Q_SIGNALS:
		/**
		 * @brief 节点被双击时发射的信号
		 * @param[in] proxy 双击的节点代理
		 */
		void nodeDoubleClicked(DA::DAPyNodeProxy* proxy);

	public:
	/**
	 * @brief 鼠标动作标记
	 */
	enum SceneActionFlag
	{
		AddRectItemAction,  ///< 开始添加矩形
		AddTextItemAction   ///< 开始添加文本
	};
	Q_ENUM(SceneActionFlag)
public:
	DAPyWorkFlowGraphicsScene(QObject* parent = 0);
	~DAPyWorkFlowGraphicsScene();
	// 设置鼠标动作，一旦设置鼠标动作，鼠标点击后就会触发此动作，continuous来标记动作结束后继续保持还是还原为无动作
	void setPreDefineSceneAction(SceneActionFlag mf);
	//===================================================
	// 背景图相关操作
	//===================================================
	// 设置背景item，如果外部调用getBackgroundPixmapItem并删除，需要通过此函数把保存的item设置为null
	void setBackgroundPixmapItem(DAGraphicsPixmapItem* item);
	DAGraphicsPixmapItem* removeBackgroundPixmapItem();
	// 允许item跟随背景图移动
	void enableItemMoveWithBackground(bool on);
	// 允许移动图元时，其它和此图元链接起来的图元跟随移动
	void setEnableItemLinkageMove(bool on);
	bool isEnableItemLinkageMove() const;
	// 是否item跟随背景图移动
	bool isEnableItemMoveWithBackground() const;
	// 添加一个背景图,如果多次调用，此函数返回的QGraphicsPixmapItem* 是一样的，也就是只会创建一个QGraphicsPixmapItem*
	DAGraphicsPixmapItem* setBackgroundPixmap(const QPixmap& pixmap);
	// 获取背景图item，如果没有设置返回一个nullptr
	DAGraphicsPixmapItem* getBackgroundPixmapItem() const;

	// 设置文本字体
	QFont getDefaultTextFont() const;
	void setDefaultTextFont(const QFont& f);
	// 设置文本颜色
	QColor getDefaultTextColor() const;
	void setDefaultTextColor(const QColor& c);

protected:
	DAGraphicsPixmapItem* ensureGetBackgroundPixmapItem();
	DAGraphicsPixmapItem* createBackgroundPixmapItem();

protected:
#if 0
	virtual void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
	virtual void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
	virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent* event) override;
	virtual void dropEvent(QGraphicsSceneDragDropEvent* event) override;
#endif

private slots:
#if DA_USE_QGRAPHICSOBJECT
	void backgroundPixmapItemXChanged();
	void backgroundPixmapItemYChanged();
#endif
	void onItemsPositionChanged(
		const QList< QGraphicsItem* >& items, const QList< QPointF >& oldPos, const QList< QPointF >& newPos
	);
	/**
	 * @brief 节点双击处理槽，弹出参数配置对话框
	 * @param[in] proxy 双击的节点代理
	 */
	void onNodeDoubleClicked(DA::DAPyNodeProxy* proxy);
	/**
	 * @brief 节点创建时连接其双击信号
	 * @param[in] item 新创建的节点图形项
	 */
	void onPyNodeItemCreated(DA::DAPyNodeGraphicsItem* item);

private:
	DAGraphicsPixmapItem* mBackgroundPixmapItem;
	SceneActionFlag mMouseAction;           ///< 鼠标动作
	bool mEnableItemMoveWithBackground;     ///< 允许item跟随背景图移动
	bool mEnableItemLinkageMove { false };  ///< 允许链接好的item进行跟随移动
	QColor mTextColor;                      ///< 文本的颜色
	QFont mTextFont;                        ///< 文本的字体
	QPointF mBackgroundPixmapItemLastPos;   ///< 背景图移动前的坐标
};
}  // end of namespace DA
#endif  // DAPYWORKFLOWGRAPHICSSCENE_H
