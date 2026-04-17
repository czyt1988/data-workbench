#ifndef DAPROPERTYPANELWIDGET_H
#define DAPROPERTYPANELWIDGET_H
#include "DACommonWidgetsAPI.h"
#include <QWidget>
#include <QString>
#include <functional>
#include "DAPropertyItemWidget.h"

namespace DA
{
/**
 * @brief 属性面板控件，用于组织和管理多个属性项
 * 
 * 内置QScrollArea，必要时显示滚动条。
 * 属性项通过addProperty()/insertProperty()添加，支持用户指定ID或面板自动分配ID。
 * 
 * @code
 * // 创建属性面板并添加属性项
 * DAPropertyPanelWidget* panel = new DAPropertyPanelWidget(this);
 * int id1 = panel->addProperty("颜色", new DAColorPickerButton(this));
 * int id2 = panel->addProperty(5, "画笔", "设置画笔样式", new DAPenEditWidget(this), DAPropertyItemWidget::BelowLayout);
 * 
 * // 遍历所有属性项
 * panel->traverseItems([](DAPropertyItemWidget* item) {
 *     item->setFrameVisible(true);
 *     return true;  // 继续遍历
 * });
 * @endcode
 * 
 * @see DAPropertyItemWidget
 */
class DACOMMONWIDGETS_API DAPropertyPanelWidget : public QWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPropertyPanelWidget)
public:
	/**
	 * @brief 遍历回调函数类型
	 * 
	 * @param item 当前遍历到的属性项
	 * @return true继续遍历，false停止遍历
	 */
	using TraverseCallback = std::function<bool(DAPropertyItemWidget* item)>;
	typedef QList<DAPropertyItemWidget*> PropertyItemList;

	explicit DAPropertyPanelWidget(QWidget* parent = nullptr);
	~DAPropertyPanelWidget();

	// === 添加属性项（末尾添加）===

	// 简化版（无description）
	int addProperty(const QString& name,
	               QWidget* editor,
	               DAPropertyItemWidget::LayoutMode mode = DAPropertyItemWidget::InlineLayout);

	// 指定ID（无description）
	int addProperty(int id,
	               const QString& name,
	               QWidget* editor,
	               DAPropertyItemWidget::LayoutMode mode = DAPropertyItemWidget::InlineLayout);

	// 完整参数
	int addProperty(const QString& name,
	               const QString& description,
	               QWidget* editor,
	               DAPropertyItemWidget::LayoutMode mode = DAPropertyItemWidget::InlineLayout);

	// 完整参数，指定ID
	int addProperty(int id,
	               const QString& name,
	               const QString& description,
	               QWidget* editor,
	               DAPropertyItemWidget::LayoutMode mode = DAPropertyItemWidget::InlineLayout);

	// === 插入属性项（指定位置）===

	int insertProperty(int index,
	                   const QString& name,
	                   QWidget* editor,
	                   DAPropertyItemWidget::LayoutMode mode = DAPropertyItemWidget::InlineLayout);

	int insertProperty(int index,
	                   int id,
	                   const QString& name,
	                   QWidget* editor,
	                   DAPropertyItemWidget::LayoutMode mode = DAPropertyItemWidget::InlineLayout);

	int insertProperty(int index,
	                   const QString& name,
	                   const QString& description,
	                   QWidget* editor,
	                   DAPropertyItemWidget::LayoutMode mode = DAPropertyItemWidget::InlineLayout);

	int insertProperty(int index,
	                   int id,
	                   const QString& name,
	                   const QString& description,
	                   QWidget* editor,
	                   DAPropertyItemWidget::LayoutMode mode = DAPropertyItemWidget::InlineLayout);

	// === 分隔项 ===

	// 添加空白间距（末尾）
	void addSpacer(int height = 8);
	// 插入空白间距
	void insertSpacer(int index, int height = 8);
	// 添加水平线分隔（末尾）
	void addSeparator();
	// 插入水平线分隔
	void insertSeparator(int index);

	// === 属性项管理 ===

	void removeProperty(int id);
	void clearProperties();
	DAPropertyItemWidget* getPropertyItem(int id) const;
	DAPropertyItemWidget* getPropertyItemAt(int index) const;
	int indexOf(int id) const;
	QList<int> propertyIds() const;
	int propertyCount() const;

	// === 遍历接口 ===

	void traverseItems(TraverseCallback callback);
	void traverseItems(TraverseCallback callback) const;
	PropertyItemList allPropertyItems() const;

	// === 面板配置 ===

	// 设置属性名标签宽度（-1表示自动计算）
	void setPropertyNameWidth(int width);
	int propertyNameWidth() const;

	// 设置属性项间距（默认4）
	void setSpacing(int spacing);
	int spacing() const;

	// 重新计算属性名标签宽度
	void recalculatePropertyNameWidth();

Q_SIGNALS:
	/**
	 * @brief 属性值变化信号
	 * @param propertyId 属性ID
	 * @note 此信号转发自DAPropertyItemWidget::valueChanged
	 */
	void propertyValueChanged(int propertyId);

private Q_SLOTS:
	void onItemValueChanged(int propertyId);

private:
	void connectItemSignals(DAPropertyItemWidget* item);
};
}  // namespace DA
#endif  // DAPROPERTYPANELWIDGET_H