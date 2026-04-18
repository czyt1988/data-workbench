#include "DAPropertyPanelContainerWidget.h"
#include "DAPropertyPanelWidget.h"
#include <QScrollArea>
#include <QVBoxLayout>

namespace DA
{

/**
 * @brief DAPropertyPanelContainerWidget的私有数据类
 */
class DAPropertyPanelContainerWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAPropertyPanelContainerWidget)

	explicit PrivateData(DAPropertyPanelContainerWidget* p)
	    : q_ptr(p)
	{
	}

	// 滚动区域
	QScrollArea* mScrollArea = nullptr;
	// 滚动区域的内容容器
	QWidget* mContentWidget = nullptr;
	// 根属性面板
	DAPropertyPanelWidget* mRootPanel = nullptr;
};

/**
 * @brief 构造函数
 * 
 * 创建容器结构：QWidget → QVBoxLayout → QScrollArea → contentWidget → QVBoxLayout → mRootPanel
 * 所有布局的margin和spacing设为0，保持紧凑布局。
 * 
 * @code
 * DAPropertyPanelContainerWidget* container = new DAPropertyPanelContainerWidget(parent);
 * @endcode
 * 
 * @param[in] parent 父控件
 * @note 自动连接根面板的propertyValueChanged信号到容器的propertyValueChanged
 */
DAPropertyPanelContainerWidget::DAPropertyPanelContainerWidget(QWidget* parent)
    : QWidget(parent)
    , DA_PIMPL_CONSTRUCT
{
	setupUI();
}

/**
 * @brief 析构函数
 * 
 * Qt父子关系自动管理内存，无需手动删除子控件。
 */
DAPropertyPanelContainerWidget::~DAPropertyPanelContainerWidget()
{
}

/**
 * @brief 设置UI结构
 * 
 * 构建嵌套的控件层次，使用QScrollArea包裹根属性面板。
 */
void DAPropertyPanelContainerWidget::setupUI()
{
	DA_D(d);

	// 创建主布局
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	// 创建滚动区域
	d->mScrollArea = new QScrollArea(this);
	d->mScrollArea->setWidgetResizable(true);
	d->mScrollArea->setFrameShape(QFrame::NoFrame);
	d->mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// 创建滚动区域的内容容器
	d->mContentWidget = new QWidget();
	QVBoxLayout* contentLayout = new QVBoxLayout(d->mContentWidget);
	contentLayout->setContentsMargins(0, 0, 0, 0);
	contentLayout->setSpacing(0);

	// 创建根属性面板
	d->mRootPanel = new DAPropertyPanelWidget(d->mContentWidget);
	contentLayout->addWidget(d->mRootPanel);

	d->mScrollArea->setWidget(d->mContentWidget);
	mainLayout->addWidget(d->mScrollArea);

	// 信号转发
	connect(d->mRootPanel, &DAPropertyPanelWidget::propertyValueChanged,
	        this, &DAPropertyPanelContainerWidget::propertyValueChanged);
}

// === 根面板访问 ===

/**
 * @brief 获取内部根属性面板
 * @return 根面板指针
 */
DAPropertyPanelWidget* DAPropertyPanelContainerWidget::rootPanel() const
{
	DA_DC(d);
	return d->mRootPanel;
}

// === 添加属性项（末尾添加）===

/**
 * @brief 添加属性项（简化版，无描述）
 * @param[in] name 属性名称
 * @param[in] editor 编辑控件
 * @param[in] mode 布局模式
 * @return 属性项ID
 */
int DAPropertyPanelContainerWidget::addProperty(const QString& name,
                                                 QWidget* editor,
                                                 DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	return d->mRootPanel->addProperty(name, editor, mode);
}

/**
 * @brief 添加属性项（指定ID，无描述）
 * @param[in] id 属性ID
 * @param[in] name 属性名称
 * @param[in] editor 编辑控件
 * @param[in] mode 布局模式
 * @return 属性项ID
 */
int DAPropertyPanelContainerWidget::addProperty(int id, const QString& name,
                                                 QWidget* editor,
                                                 DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	return d->mRootPanel->addProperty(id, name, editor, mode);
}

/**
 * @brief 添加属性项（完整参数）
 * @param[in] name 属性名称
 * @param[in] description 属性描述
 * @param[in] editor 编辑控件
 * @param[in] mode 布局模式
 * @return 属性项ID
 */
int DAPropertyPanelContainerWidget::addProperty(const QString& name,
                                                 const QString& description,
                                                 QWidget* editor,
                                                 DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	return d->mRootPanel->addProperty(name, description, editor, mode);
}

/**
 * @brief 添加属性项（完整参数，指定ID）
 * @param[in] id 属性ID
 * @param[in] name 属性名称
 * @param[in] description 属性描述
 * @param[in] editor 编辑控件
 * @param[in] mode 布局模式
 * @return 属性项ID
 */
int DAPropertyPanelContainerWidget::addProperty(int id, const QString& name,
                                                 const QString& description,
                                                 QWidget* editor,
                                                 DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	return d->mRootPanel->addProperty(id, name, description, editor, mode);
}

// === 插入属性项（指定位置）===

/**
 * @brief 在指定位置插入属性项（无描述）
 * @param[in] index 插入位置索引
 * @param[in] name 属性名称
 * @param[in] editor 编辑控件
 * @param[in] mode 布局模式
 * @return 属性项ID
 */
int DAPropertyPanelContainerWidget::insertProperty(int index, const QString& name,
                                                    QWidget* editor,
                                                    DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	return d->mRootPanel->insertProperty(index, name, editor, mode);
}

/**
 * @brief 在指定位置插入属性项（指定ID，无描述）
 * @param[in] index 插入位置索引
 * @param[in] id 属性ID
 * @param[in] name 属性名称
 * @param[in] editor 编辑控件
 * @param[in] mode 布局模式
 * @return 属性项ID
 */
int DAPropertyPanelContainerWidget::insertProperty(int index, int id, const QString& name,
                                                    QWidget* editor,
                                                    DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	return d->mRootPanel->insertProperty(index, id, name, editor, mode);
}

/**
 * @brief 在指定位置插入属性项（带描述）
 * @param[in] index 插入位置索引
 * @param[in] name 属性名称
 * @param[in] description 属性描述
 * @param[in] editor 编辑控件
 * @param[in] mode 布局模式
 * @return 属性项ID
 */
int DAPropertyPanelContainerWidget::insertProperty(int index, const QString& name,
                                                    const QString& description,
                                                    QWidget* editor,
                                                    DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	return d->mRootPanel->insertProperty(index, name, description, editor, mode);
}

/**
 * @brief 在指定位置插入属性项（指定ID，带描述）
 * @param[in] index 插入位置索引
 * @param[in] id 属性ID
 * @param[in] name 属性名称
 * @param[in] description 属性描述
 * @param[in] editor 编辑控件
 * @param[in] mode 布局模式
 * @return 属性项ID
 */
int DAPropertyPanelContainerWidget::insertProperty(int index, int id, const QString& name,
                                                    const QString& description,
                                                    QWidget* editor,
                                                    DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	return d->mRootPanel->insertProperty(index, id, name, description, editor, mode);
}

// === 分隔项 ===

/**
 * @brief 添加空白间距项
 * @param[in] height 间距高度（像素）
 */
void DAPropertyPanelContainerWidget::addSpacer(int height)
{
	DA_D(d);
	d->mRootPanel->addSpacer(height);
}

/**
 * @brief 在指定位置插入空白间距项
 * @param[in] index 插入位置索引
 * @param[in] height 间距高度（像素）
 */
void DAPropertyPanelContainerWidget::insertSpacer(int index, int height)
{
	DA_D(d);
	d->mRootPanel->insertSpacer(index, height);
}

/**
 * @brief 添加水平线分隔项
 */
void DAPropertyPanelContainerWidget::addSeparator()
{
	DA_D(d);
	d->mRootPanel->addSeparator();
}

/**
 * @brief 在指定位置插入水平线分隔项
 * @param[in] index 插入位置索引
 */
void DAPropertyPanelContainerWidget::insertSeparator(int index)
{
	DA_D(d);
	d->mRootPanel->insertSeparator(index);
}

// === 分组管理 ===

/// addCollapsibleGroup 代理
int DAPropertyPanelContainerWidget::addCollapsibleGroup(const QString& title)
{
	DA_D(d);
	return d->mRootPanel->addCollapsibleGroup(title);
}

/// endGroup 代理
void DAPropertyPanelContainerWidget::endGroup()
{
	DA_D(d);
	d->mRootPanel->endGroup();
}

/// addSubPanel 代理
DAPropertyPanelWidget* DAPropertyPanelContainerWidget::addSubPanel(int id, const QString& groupName)
{
	DA_D(d);
	return d->mRootPanel->addSubPanel(id, groupName);
}

/// getSubPanel 代理
DAPropertyPanelWidget* DAPropertyPanelContainerWidget::getSubPanel(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getSubPanel(id);
}

/// getSubPanelId 代理
int DAPropertyPanelContainerWidget::getSubPanelId(DAPropertyPanelWidget* subPanel) const
{
	DA_DC(d);
	return d->mRootPanel->getSubPanelId(subPanel);
}

/// getGroupPanel 代理
DAPropertyPanelWidget* DAPropertyPanelContainerWidget::getGroupPanel(int groupId) const
{
	DA_DC(d);
	return d->mRootPanel->getGroupPanel(groupId);
}

/// isGroupExpanded 代理
bool DAPropertyPanelContainerWidget::isGroupExpanded(int groupId) const
{
	DA_DC(d);
	return d->mRootPanel->isGroupExpanded(groupId);
}

/// setGroupExpanded 代理
void DAPropertyPanelContainerWidget::setGroupExpanded(int groupId, bool expanded)
{
	DA_D(d);
	d->mRootPanel->setGroupExpanded(groupId, expanded);
}

// === 属性项管理 ===

/// removeProperty 代理
void DAPropertyPanelContainerWidget::removeProperty(int id)
{
	DA_D(d);
	d->mRootPanel->removeProperty(id);
}

/// clearProperties 代理
void DAPropertyPanelContainerWidget::clearProperties()
{
	DA_D(d);
	d->mRootPanel->clearProperties();
}

/// getPropertyItem 代理
DAPropertyItemWidget* DAPropertyPanelContainerWidget::getPropertyItem(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getPropertyItem(id);
}

/// getPropertyItemAt 代理
DAPropertyItemWidget* DAPropertyPanelContainerWidget::getPropertyItemAt(int index) const
{
	DA_DC(d);
	return d->mRootPanel->getPropertyItemAt(index);
}

/// indexOf 代理
int DAPropertyPanelContainerWidget::indexOf(int id) const
{
	DA_DC(d);
	return d->mRootPanel->indexOf(id);
}

/// propertyIds 代理
QList<int> DAPropertyPanelContainerWidget::propertyIds() const
{
	DA_DC(d);
	return d->mRootPanel->propertyIds();
}

/// propertyCount 代理
int DAPropertyPanelContainerWidget::propertyCount() const
{
	DA_DC(d);
	return d->mRootPanel->propertyCount();
}

// === 遍历接口 ===

/// traverseItems（非常量）代理
void DAPropertyPanelContainerWidget::traverseItems(TraverseCallback callback)
{
	DA_D(d);
	d->mRootPanel->traverseItems(callback);
}

/// traverseItems（常量）代理
void DAPropertyPanelContainerWidget::traverseItems(TraverseCallback callback) const
{
	DA_DC(d);
	d->mRootPanel->traverseItems(callback);
}

/// allPropertyItems 代理
DAPropertyPanelContainerWidget::PropertyItemList DAPropertyPanelContainerWidget::allPropertyItems() const
{
	DA_DC(d);
	return d->mRootPanel->allPropertyItems();
}

// === 面板配置 ===

/// setPropertyNameWidth 代理
void DAPropertyPanelContainerWidget::setPropertyNameWidth(int width)
{
	DA_D(d);
	d->mRootPanel->setPropertyNameWidth(width);
}

/// propertyNameWidth 代理
int DAPropertyPanelContainerWidget::propertyNameWidth() const
{
	DA_DC(d);
	return d->mRootPanel->propertyNameWidth();
}

/// setSpacing 代理
void DAPropertyPanelContainerWidget::setSpacing(int spacing)
{
	DA_D(d);
	d->mRootPanel->setSpacing(spacing);
}

/// spacing 代理
int DAPropertyPanelContainerWidget::spacing() const
{
	DA_DC(d);
	return d->mRootPanel->spacing();
}

/// recalculatePropertyNameWidth 代理
void DAPropertyPanelContainerWidget::recalculatePropertyNameWidth()
{
	DA_D(d);
	d->mRootPanel->recalculatePropertyNameWidth();
}

// === 便捷属性添加方法 ===

/// addColorProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addColorProperty(int id, const QString& name, const QColor& color)
{
	DA_D(d);
	return d->mRootPanel->addColorProperty(id, name, color);
}

/// addColorProperty（无ID）代理
int DAPropertyPanelContainerWidget::addColorProperty(const QString& name, const QColor& color)
{
	DA_D(d);
	return d->mRootPanel->addColorProperty(name, color);
}

/// addFontProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addFontProperty(int id, const QString& name, const QFont& font)
{
	DA_D(d);
	return d->mRootPanel->addFontProperty(id, name, font);
}

/// addFontProperty（无ID）代理
int DAPropertyPanelContainerWidget::addFontProperty(const QString& name, const QFont& font)
{
	DA_D(d);
	return d->mRootPanel->addFontProperty(name, font);
}

/// addBrushProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addBrushProperty(int id, const QString& name, const QBrush& brush)
{
	DA_D(d);
	return d->mRootPanel->addBrushProperty(id, name, brush);
}

/// addBrushProperty（无ID）代理
int DAPropertyPanelContainerWidget::addBrushProperty(const QString& name, const QBrush& brush)
{
	DA_D(d);
	return d->mRootPanel->addBrushProperty(name, brush);
}

/// addPenProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addPenProperty(int id, const QString& name, const QPen& pen)
{
	DA_D(d);
	return d->mRootPanel->addPenProperty(id, name, pen);
}

/// addPenProperty（无ID）代理
int DAPropertyPanelContainerWidget::addPenProperty(const QString& name, const QPen& pen)
{
	DA_D(d);
	return d->mRootPanel->addPenProperty(name, pen);
}

/// addIntProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addIntProperty(int id, const QString& name, int value, int min, int max)
{
	DA_D(d);
	return d->mRootPanel->addIntProperty(id, name, value, min, max);
}

/// addIntProperty（无ID）代理
int DAPropertyPanelContainerWidget::addIntProperty(const QString& name, int value, int min, int max)
{
	DA_D(d);
	return d->mRootPanel->addIntProperty(name, value, min, max);
}

/// addDoubleProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addDoubleProperty(int id, const QString& name, double value, double min, double max, int decimals)
{
	DA_D(d);
	return d->mRootPanel->addDoubleProperty(id, name, value, min, max, decimals);
}

/// addDoubleProperty（无ID）代理
int DAPropertyPanelContainerWidget::addDoubleProperty(const QString& name, double value, double min, double max, int decimals)
{
	DA_D(d);
	return d->mRootPanel->addDoubleProperty(name, value, min, max, decimals);
}

/// addBoolProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addBoolProperty(int id, const QString& name, bool checked)
{
	DA_D(d);
	return d->mRootPanel->addBoolProperty(id, name, checked);
}

/// addBoolProperty（无ID）代理
int DAPropertyPanelContainerWidget::addBoolProperty(const QString& name, bool checked)
{
	DA_D(d);
	return d->mRootPanel->addBoolProperty(name, checked);
}

/// addStringProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addStringProperty(int id, const QString& name, const QString& text)
{
	DA_D(d);
	return d->mRootPanel->addStringProperty(id, name, text);
}

/// addStringProperty（无ID）代理
int DAPropertyPanelContainerWidget::addStringProperty(const QString& name, const QString& text)
{
	DA_D(d);
	return d->mRootPanel->addStringProperty(name, text);
}

/// addEnumProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addEnumProperty(int id, const QString& name, const QStringList& items, const QList<int>& dataValues, int currentIndex)
{
	DA_D(d);
	return d->mRootPanel->addEnumProperty(id, name, items, dataValues, currentIndex);
}

/// addEnumProperty（无ID）代理
int DAPropertyPanelContainerWidget::addEnumProperty(const QString& name, const QStringList& items, const QList<int>& dataValues, int currentIndex)
{
	DA_D(d);
	return d->mRootPanel->addEnumProperty(name, items, dataValues, currentIndex);
}

/// addAlignmentProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addAlignmentProperty(int id, const QString& name, Qt::Alignment alignment)
{
	DA_D(d);
	return d->mRootPanel->addAlignmentProperty(id, name, alignment);
}

/// addAlignmentProperty（无ID）代理
int DAPropertyPanelContainerWidget::addAlignmentProperty(const QString& name, Qt::Alignment alignment)
{
	DA_D(d);
	return d->mRootPanel->addAlignmentProperty(name, alignment);
}

/// addAlignmentPositionProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addAlignmentPositionProperty(int id, const QString& name, Qt::Alignment alignment)
{
	DA_D(d);
	return d->mRootPanel->addAlignmentPositionProperty(id, name, alignment);
}

/// addAlignmentPositionProperty（无ID）代理
int DAPropertyPanelContainerWidget::addAlignmentPositionProperty(const QString& name, Qt::Alignment alignment)
{
	DA_D(d);
	return d->mRootPanel->addAlignmentPositionProperty(name, alignment);
}

/// addFilePathProperty（指定ID）代理
int DAPropertyPanelContainerWidget::addFilePathProperty(int id, const QString& name, const QString& filter)
{
	DA_D(d);
	return d->mRootPanel->addFilePathProperty(id, name, filter);
}

/// addFilePathProperty（无ID）代理
int DAPropertyPanelContainerWidget::addFilePathProperty(const QString& name, const QString& filter)
{
	DA_D(d);
	return d->mRootPanel->addFilePathProperty(name, filter);
}

// === 分组标签 ===

/// addGroupLabel 代理
void DAPropertyPanelContainerWidget::addGroupLabel(const QString& text)
{
	DA_D(d);
	d->mRootPanel->addGroupLabel(text);
}

/// insertGroupLabel 代理
void DAPropertyPanelContainerWidget::insertGroupLabel(int index, const QString& text)
{
	DA_D(d);
	d->mRootPanel->insertGroupLabel(index, text);
}

// === 值读写方法 ===

/// getColorValue 代理
QColor DAPropertyPanelContainerWidget::getColorValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getColorValue(id);
}

/// setColorValue 代理
void DAPropertyPanelContainerWidget::setColorValue(int id, const QColor& color)
{
	DA_D(d);
	d->mRootPanel->setColorValue(id, color);
}

/// getFontValue 代理
QFont DAPropertyPanelContainerWidget::getFontValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getFontValue(id);
}

/// setFontValue 代理
void DAPropertyPanelContainerWidget::setFontValue(int id, const QFont& font)
{
	DA_D(d);
	d->mRootPanel->setFontValue(id, font);
}

/// getBrushValue 代理
QBrush DAPropertyPanelContainerWidget::getBrushValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getBrushValue(id);
}

/// setBrushValue 代理
void DAPropertyPanelContainerWidget::setBrushValue(int id, const QBrush& brush)
{
	DA_D(d);
	d->mRootPanel->setBrushValue(id, brush);
}

/// getPenValue 代理
QPen DAPropertyPanelContainerWidget::getPenValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getPenValue(id);
}

/// setPenValue 代理
void DAPropertyPanelContainerWidget::setPenValue(int id, const QPen& pen)
{
	DA_D(d);
	d->mRootPanel->setPenValue(id, pen);
}

/// getIntValue 代理
int DAPropertyPanelContainerWidget::getIntValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getIntValue(id);
}

/// setIntValue 代理
void DAPropertyPanelContainerWidget::setIntValue(int id, int value)
{
	DA_D(d);
	d->mRootPanel->setIntValue(id, value);
}

/// getDoubleValue 代理
double DAPropertyPanelContainerWidget::getDoubleValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getDoubleValue(id);
}

/// setDoubleValue 代理
void DAPropertyPanelContainerWidget::setDoubleValue(int id, double value)
{
	DA_D(d);
	d->mRootPanel->setDoubleValue(id, value);
}

/// getBoolValue 代理
bool DAPropertyPanelContainerWidget::getBoolValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getBoolValue(id);
}

/// setBoolValue 代理
void DAPropertyPanelContainerWidget::setBoolValue(int id, bool checked)
{
	DA_D(d);
	d->mRootPanel->setBoolValue(id, checked);
}

/// getStringValue 代理
QString DAPropertyPanelContainerWidget::getStringValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getStringValue(id);
}

/// setStringValue 代理
void DAPropertyPanelContainerWidget::setStringValue(int id, const QString& text)
{
	DA_D(d);
	d->mRootPanel->setStringValue(id, text);
}

/// getEnumValue 代理
int DAPropertyPanelContainerWidget::getEnumValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getEnumValue(id);
}

/// setEnumValue 代理
void DAPropertyPanelContainerWidget::setEnumValue(int id, int index)
{
	DA_D(d);
	d->mRootPanel->setEnumValue(id, index);
}

/// getAlignmentValue 代理
Qt::Alignment DAPropertyPanelContainerWidget::getAlignmentValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getAlignmentValue(id);
}

/// setAlignmentValue 代理
void DAPropertyPanelContainerWidget::setAlignmentValue(int id, Qt::Alignment alignment)
{
	DA_D(d);
	d->mRootPanel->setAlignmentValue(id, alignment);
}

/// getAlignmentPositionValue 代理
Qt::Alignment DAPropertyPanelContainerWidget::getAlignmentPositionValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getAlignmentPositionValue(id);
}

/// setAlignmentPositionValue 代理
void DAPropertyPanelContainerWidget::setAlignmentPositionValue(int id, Qt::Alignment alignment)
{
	DA_D(d);
	d->mRootPanel->setAlignmentPositionValue(id, alignment);
}

/// getFilePathValue 代理
QString DAPropertyPanelContainerWidget::getFilePathValue(int id) const
{
	DA_DC(d);
	return d->mRootPanel->getFilePathValue(id);
}

/// setFilePathValue 代理
void DAPropertyPanelContainerWidget::setFilePathValue(int id, const QString& path)
{
	DA_D(d);
	d->mRootPanel->setFilePathValue(id, path);
}

// === 可见性与状态控制 ===

/// setPropertyVisible 代理
void DAPropertyPanelContainerWidget::setPropertyVisible(int id, bool visible)
{
	DA_D(d);
	d->mRootPanel->setPropertyVisible(id, visible);
}

/// setPropertyEnabled 代理
void DAPropertyPanelContainerWidget::setPropertyEnabled(int id, bool enabled)
{
	DA_D(d);
	d->mRootPanel->setPropertyEnabled(id, enabled);
}

/// propertyExists 代理
bool DAPropertyPanelContainerWidget::propertyExists(int id) const
{
	DA_DC(d);
	return d->mRootPanel->propertyExists(id);
}

}  // namespace DA
