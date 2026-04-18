#ifndef DAPROPERTYPANELCONTAINERWIDGET_H
#define DAPROPERTYPANELCONTAINERWIDGET_H

#include "DACommonWidgetsAPI.h"
#include <QWidget>
#include <QString>
#include <QPen>
#include <QMap>
#include <functional>
#include "DAPropertyItemWidget.h"
#include "DAPropertyPanelWidget.h"

class QScrollArea;

namespace DA
{

/**
 * @brief 属性面板容器控件，提供滚动区域并代理DAPropertyPanelWidget的完整API
 * 
 * 该类封装了QScrollArea和根DAPropertyPanelWidget，将所有公共API方法代理到根面板。
 * 用户无需关注容器内部结构，直接使用与DAPropertyPanelWidget相同的接口即可。
 * 
 * @code
 * // 创建容器并添加属性
 * DAPropertyPanelContainerWidget* container = new DAPropertyPanelContainerWidget(this);
 * int id = container->addColorProperty("线条颜色", Qt::red);
 * container->addIntProperty("线条宽度", 1, 1, 10);
 * 
 * // 连接属性值变化信号
 * connect(container, &DAPropertyPanelContainerWidget::propertyValueChanged,
 *         this, &MyClass::onPropertyChanged);
 * @endcode
 * 
 * @see DAPropertyPanelWidget
 */
class DACOMMONWIDGETS_API DAPropertyPanelContainerWidget : public QWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPropertyPanelContainerWidget)

	// 遍历回调函数类型（转发自DAPropertyPanelWidget）
	using TraverseCallback = DAPropertyPanelWidget::TraverseCallback;
	typedef QList<DAPropertyItemWidget*> PropertyItemList;

public:
	explicit DAPropertyPanelContainerWidget(QWidget* parent = nullptr);
	~DAPropertyPanelContainerWidget();

	// === 根面板访问 ===

	/// @brief 获取内部根属性面板
	/// @return 根面板指针
	DAPropertyPanelWidget* rootPanel() const;

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

	void addSpacer(int height = 8);
	void insertSpacer(int index, int height = 8);
	void addSeparator();
	void insertSeparator(int index);

	// === 分组管理 ===

	int addCollapsibleGroup(const QString& title);
	void endGroup();

	DAPropertyPanelWidget* addSubPanel(int id, const QString& groupName);
	DAPropertyPanelWidget* getSubPanel(int id) const;
	int getSubPanelId(DAPropertyPanelWidget* subPanel) const;
	DAPropertyPanelWidget* getGroupPanel(int groupId) const;

	bool isGroupExpanded(int groupId) const;
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

	void setPropertyNameWidth(int width);
	int propertyNameWidth() const;

	void setSpacing(int spacing);
	int spacing() const;

	void recalculatePropertyNameWidth();

	// === 便捷属性添加方法 ===

	// 颜色
	int addColorProperty(int id, const QString& name, const QColor& color = Qt::black);
	int addColorProperty(const QString& name, const QColor& color = Qt::black);

	// 字体
	int addFontProperty(int id, const QString& name, const QFont& font = QFont());
	int addFontProperty(const QString& name, const QFont& font = QFont());

	// 画刷
	int addBrushProperty(int id, const QString& name, const QBrush& brush = QBrush());
	int addBrushProperty(const QString& name, const QBrush& brush = QBrush());

	// 画笔
	int addPenProperty(int id, const QString& name, const QPen& pen = QPen());
	int addPenProperty(const QString& name, const QPen& pen = QPen());

	// 整数
	int addIntProperty(int id, const QString& name, int value = 0, int min = 0, int max = 100);
	int addIntProperty(const QString& name, int value = 0, int min = 0, int max = 100);

	// 浮点
	int addDoubleProperty(int id, const QString& name, double value = 0.0, double min = 0.0, double max = 100.0, int decimals = 2);
	int addDoubleProperty(const QString& name, double value = 0.0, double min = 0.0, double max = 100.0, int decimals = 2);

	// 布尔
	int addBoolProperty(int id, const QString& name, bool checked = false);
	int addBoolProperty(const QString& name, bool checked = false);

	// 字符串
	int addStringProperty(int id, const QString& name, const QString& text = QString());
	int addStringProperty(const QString& name, const QString& text = QString());

	// 枚举
	int addEnumProperty(int id, const QString& name, const QStringList& items, const QList<int>& dataValues = QList<int>(), int currentIndex = 0);
	int addEnumProperty(const QString& name, const QStringList& items, const QList<int>& dataValues = QList<int>(), int currentIndex = 0);

	// 对齐
	int addAlignmentProperty(int id, const QString& name, Qt::Alignment alignment = Qt::AlignLeft);
	int addAlignmentProperty(const QString& name, Qt::Alignment alignment = Qt::AlignLeft);

	// 对齐位置
	int addAlignmentPositionProperty(int id, const QString& name, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop);
	int addAlignmentPositionProperty(const QString& name, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop);

	// 文件路径
	int addFilePathProperty(int id, const QString& name, const QString& filter = QString());
	int addFilePathProperty(const QString& name, const QString& filter = QString());

	// === 分组标签 ===

	void addGroupLabel(const QString& text);
	void insertGroupLabel(int index, const QString& text);

	// === 值读写方法 ===

	// 颜色
	QColor getColorValue(int id) const;
	void setColorValue(int id, const QColor& color);

	// 字体
	QFont getFontValue(int id) const;
	void setFontValue(int id, const QFont& font);

	// 画刷
	QBrush getBrushValue(int id) const;
	void setBrushValue(int id, const QBrush& brush);

	// 画笔
	QPen getPenValue(int id) const;
	void setPenValue(int id, const QPen& pen);

	// 整数
	int getIntValue(int id) const;
	void setIntValue(int id, int value);

	// 浮点
	double getDoubleValue(int id) const;
	void setDoubleValue(int id, double value);

	// 布尔
	bool getBoolValue(int id) const;
	void setBoolValue(int id, bool checked);

	// 字符串
	QString getStringValue(int id) const;
	void setStringValue(int id, const QString& text);

	// 枚举
	int getEnumValue(int id) const;
	void setEnumValue(int id, int index);

	// 对齐
	Qt::Alignment getAlignmentValue(int id) const;
	void setAlignmentValue(int id, Qt::Alignment alignment);

	// 对齐位置
	Qt::Alignment getAlignmentPositionValue(int id) const;
	void setAlignmentPositionValue(int id, Qt::Alignment alignment);

	// 文件路径
	QString getFilePathValue(int id) const;
	void setFilePathValue(int id, const QString& path);

	// === 可见性与状态控制 ===

	void setPropertyVisible(int id, bool visible);
	void setPropertyEnabled(int id, bool enabled);
	bool propertyExists(int id) const;

Q_SIGNALS:
	/**
	 * @brief 属性值变化信号（转发自根面板）
	 * @param propertyId 属性ID
	 */
	void propertyValueChanged(int propertyId);

private:
	void setupUI();
};

}  // namespace DA

#endif  // DAPROPERTYPANELCONTAINERWIDGET_H
