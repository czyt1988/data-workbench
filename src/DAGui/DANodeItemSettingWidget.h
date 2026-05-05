#ifndef DANODEITEMSETTINGWIDGET_H
#define DANODEITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QSizeF>
#include "DAPyNodeGraphicsItem.h"

namespace DA
{
class DAPyWorkFlowGraphicsScene;
class DAGraphicsResizeableItem;
class DAPropertyPanelContainerWidget;

/**
 * @brief 节点图形项设置面板
 *
 * 使用DAPropertyPanelContainerWidget构建属性面板，提供节点图形项的位置、尺寸、旋转、状态等属性编辑。
 * 支持redo/undo操作（通过设置DAPyWorkFlowGraphicsScene）。
 *
 * @see DAPropertyPanelContainerWidget
 */
class DAGUI_API DANodeItemSettingWidget : public QWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DANodeItemSettingWidget)
public:
	/**
	 * @brief 属性ID枚举
	 */
	enum PropertyId {
		PropX               = 1,  ///< X坐标
		PropY               = 2,  ///< Y坐标
		PropWidth           = 3,  ///< 宽度
		PropHeight          = 4,  ///< 高度
		PropRotation        = 5,  ///< 旋转角度
		PropMovable         = 6,  ///< 可移动
		PropResizable       = 7,  ///< 可缩放
		PropTooltip         = 8,  ///< 提示文本
		PropLockAspectRatio = 9   ///< 锁定纵横比
	};

	explicit DANodeItemSettingWidget(QWidget* parent = nullptr);
	~DANodeItemSettingWidget();
	// 设置需要配置的item
	void setItem(DAGraphicsResizeableItem* item);
	// 获取维护的item
	DAGraphicsResizeableItem* getItem() const;
	// 设置了DAGraphicsScene 能实现redo/undo
	void setScene(DAPyWorkFlowGraphicsScene* sc);
	// 更新
	void updateData();
	// 更新位置信息
	void updatePosition();
	// 更新旋转信息
	void updateRotation();
	// 更新body信息
	void updateBodySize();
	// 更新item的状态
	void updateItemState();
	// 更新连接点的位置
	void updateLinkPointLocation();
private slots:
	void onPropertyValueChanged(int propertyId);
	void onNodeItemsRemoved(const QList< DA::DAPyNodeGraphicsItem* >& items);
	void onButtonGroupClicked(int id);
private:
	void resetValue();
	void buildPanel();
	QWidget* createLinkPointWidget();
};
}
#endif  // DANODEITEMSETTINGWIDGET_H