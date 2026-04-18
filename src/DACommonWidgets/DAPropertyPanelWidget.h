#ifndef DAPROPERTYPANELWIDGET_H
#define DAPROPERTYPANELWIDGET_H
#include "DACommonWidgetsAPI.h"
#include <QWidget>
#include <QString>
#include <QPen>
#include <QMap>
#include <functional>
#include "DAPropertyItemWidget.h"

namespace DA
{
// Forward declaration
class DACollapsiblePanel;

/**
 * @brief 属性面板控件，用于组织和管理多个属性项
 * 
 * 支持分组折叠和嵌套子面板。属性项通过addProperty()/insertProperty()添加，
 * 支持用户指定ID或面板自动分配ID。可通过addCollapsibleGroup()创建可折叠分组，
 * 后续添加的属性自动归入当前分组。
 * 
 * @code
 * // 创建属性面板并添加属性项
 * DAPropertyPanelWidget* panel = new DAPropertyPanelWidget(this);
 * int id1 = panel->addProperty("颜色", new DAColorPickerButton(this));
 * int id2 = panel->addProperty(5, "画笔", "设置画笔样式", new DAPenEditWidget(this), DAPropertyItemWidget::BelowLayout);
 * 
 * // 使用折叠分组
 * int groupId = panel->addCollapsibleGroup("外观设置");
 * panel->addColorProperty("线条颜色", Qt::red);
 * panel->addIntProperty("线条宽度", 1, 1, 10);
 * panel->endGroup();
 * 
 * // 遍历所有属性项
 * panel->traverseItems([](DAPropertyItemWidget* item) {
 *     item->setFrameVisible(true);
 *     return true;  // 继续遍历
 * });
 * @endcode
 * 
 * @see DAPropertyItemWidget, DACollapsiblePanel
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

	// 添加空白间距（末尾）— 始终添加到根布局，不进入当前分组
	void addSpacer(int height = 8);
	// 插入空白间距
	void insertSpacer(int index, int height = 8);
	// 添加水平线分隔（末尾）— 始终添加到根布局，不进入当前分组
	void addSeparator();
	// 插入水平线分隔
	void insertSeparator(int index);

	// === 分组管理 ===

	/// @brief 添加可折叠分组，后续addXxxProperty自动添加到该分组
	/// @param[in] title 分组标题
	/// @return 分组ID（从1开始递增）
	int addCollapsibleGroup(const QString& title);

	/// @brief 结束当前分组，后续addXxxProperty回到根面板
	void endGroup();

	/// @brief 添加嵌套子面板（带ID映射和信号转发）
	/// @param[in] id 子面板ID（由调用者指定）
	/// @param[in] groupName 子面板标题
	/// @return 子面板指针
	DAPropertyPanelWidget* addSubPanel(int id, const QString& groupName);

	/// @brief 根据ID获取子面板
	/// @param[in] id 子面板ID
	/// @return 子面板指针，不存在则返回nullptr
	DAPropertyPanelWidget* getSubPanel(int id) const;

	/// @brief 获取子面板对应的ID
	/// @param[in] subPanel 子面板指针
	/// @return 子面板ID，不存在则返回-1
	int getSubPanelId(DAPropertyPanelWidget* subPanel) const;

	/// @brief 获取分组面板指针
	/// @param[in] groupId 分组ID
	/// @return 分组面板指针，不存在则返回nullptr
	DAPropertyPanelWidget* getGroupPanel(int groupId) const;

	/// @brief 获取分组展开状态
	/// @param[in] groupId 分组ID
	/// @return true为展开，false为收起
	bool isGroupExpanded(int groupId) const;

	/// @brief 设置分组展开状态
	/// @param[in] groupId 分组ID
	/// @param[in] expanded true为展开，false为收起
	void setGroupExpanded(int groupId, bool expanded);

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

	// === 便捷属性添加方法 ===

	// 添加颜色属性
	int addColorProperty(int id, const QString& name, const QColor& color = Qt::black);
	int addColorProperty(const QString& name, const QColor& color = Qt::black);

	// 添加字体属性
	int addFontProperty(int id, const QString& name, const QFont& font = QFont());
	int addFontProperty(const QString& name, const QFont& font = QFont());

	// 添加画刷属性
	int addBrushProperty(int id, const QString& name, const QBrush& brush = QBrush());
	int addBrushProperty(const QString& name, const QBrush& brush = QBrush());

	// 添加画笔属性
	int addPenProperty(int id, const QString& name, const QPen& pen = QPen());
	int addPenProperty(const QString& name, const QPen& pen = QPen());

	// 添加整数属性
	int addIntProperty(int id, const QString& name, int value = 0, int min = 0, int max = 100);
	int addIntProperty(const QString& name, int value = 0, int min = 0, int max = 100);

	// 添加浮点属性
	int addDoubleProperty(int id, const QString& name, double value = 0.0, double min = 0.0, double max = 100.0, int decimals = 2);
	int addDoubleProperty(const QString& name, double value = 0.0, double min = 0.0, double max = 100.0, int decimals = 2);

	// 添加布尔属性
	int addBoolProperty(int id, const QString& name, bool checked = false);
	int addBoolProperty(const QString& name, bool checked = false);

	// 添加字符串属性
	int addStringProperty(int id, const QString& name, const QString& text = QString());
	int addStringProperty(const QString& name, const QString& text = QString());

	// 添加枚举属性（下拉框）
	int addEnumProperty(int id, const QString& name, const QStringList& items, const QList<int>& dataValues = QList<int>(), int currentIndex = 0);
	int addEnumProperty(const QString& name, const QStringList& items, const QList<int>& dataValues = QList<int>(), int currentIndex = 0);

	// 添加对齐属性
	int addAlignmentProperty(int id, const QString& name, Qt::Alignment alignment = Qt::AlignLeft);
	int addAlignmentProperty(const QString& name, Qt::Alignment alignment = Qt::AlignLeft);

	// 添加对齐位置属性
	int addAlignmentPositionProperty(int id, const QString& name, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop);
	int addAlignmentPositionProperty(const QString& name, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop);

	// 添加文件路径属性
	int addFilePathProperty(int id, const QString& name, const QString& filter = QString());
	int addFilePathProperty(const QString& name, const QString& filter = QString());

	// === 分组标签 ===

	/// @deprecated 建议使用addCollapsibleGroup代替，此方法仅创建装饰性标签不具备折叠功能
	void addGroupLabel(const QString& text);
	void insertGroupLabel(int index, const QString& text);

	// === 值读写方法 ===

	// 颜色值读写
	QColor getColorValue(int id) const;
	void setColorValue(int id, const QColor& color);

	// 字体值读写
	QFont getFontValue(int id) const;
	void setFontValue(int id, const QFont& font);

	// 画刷值读写
	QBrush getBrushValue(int id) const;
	void setBrushValue(int id, const QBrush& brush);

	// 画笔值读写
	QPen getPenValue(int id) const;
	void setPenValue(int id, const QPen& pen);

	// 整数值读写
	int getIntValue(int id) const;
	void setIntValue(int id, int value);

	// 浮点值读写
	double getDoubleValue(int id) const;
	void setDoubleValue(int id, double value);

	// 布尔值读写
	bool getBoolValue(int id) const;
	void setBoolValue(int id, bool checked);

	// 字符串值读写
	QString getStringValue(int id) const;
	void setStringValue(int id, const QString& text);

	// 枚举值读写（下拉框索引）
	int getEnumValue(int id) const;
	void setEnumValue(int id, int index);

	// 对齐值读写
	Qt::Alignment getAlignmentValue(int id) const;
	void setAlignmentValue(int id, Qt::Alignment alignment);

	// 对齐位置值读写
	Qt::Alignment getAlignmentPositionValue(int id) const;
	void setAlignmentPositionValue(int id, Qt::Alignment alignment);

	// 文件路径值读写
	QString getFilePathValue(int id) const;
	void setFilePathValue(int id, const QString& path);

	// === 可见性与状态控制 ===

	// 设置属性可见性
	void setPropertyVisible(int id, bool visible);
	// 设置属性启用状态
	void setPropertyEnabled(int id, bool enabled);
	// 检查属性是否存在
	bool propertyExists(int id) const;

Q_SIGNALS:
	/**
	 * @brief 属性值变化信号
	 * @param propertyId 属性ID
	 * @note 此信号转发自DAPropertyItemWidget::valueChanged或子面板的propertyValueChanged
	 */
	void propertyValueChanged(int propertyId);

private Q_SLOTS:
	void onItemValueChanged(int propertyId);

private:
	void connectItemSignals(DAPropertyItemWidget* item);

	/// @brief 获取当前目标面板（有活动分组时返回分组面板，否则返回根面板）
	DAPropertyPanelWidget* getTargetPanel() const;
};
}  // namespace DA
#endif  // DAPROPERTYPANELWIDGET_H
