#include "DAPropertyPanelWidget.h"
#include "DACollapsiblePanel.h"
#include <QVBoxLayout>
#include <QFrame>
#include <QFontMetrics>
#include <QSpacerItem>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSignalBlocker>
#include "DAGlobals.h"

// 编辑器Widget头文件（仅在cpp中引入）
#include "DAColorPickerButton.h"
#include "DAFontEditPannelWidget.h"
#include "DABrushEditWidget.h"
#include "DAPenEditWidget.h"
#include "DAAligmentEditWidget.h"
#include "DAAligmentPositionEditWidget.h"
#include "DAFilePathEditWidget.h"

namespace DA
{
class DAPropertyPanelWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAPropertyPanelWidget)
public:
	PrivateData(DAPropertyPanelWidget* p);

	QMap<int, DAPropertyItemWidget*> mPropertyItems;  // ID -> Widget映射
	QList<QWidget*> mWidgetList;                      // 保持添加顺序（包括分隔项）
	int mNextAutoId = 1;                               // 自动分配ID计数器
	int mPropertyNameWidth = -1;                       // -1表示自动计算
	int mSpacing = 4;                                  // 属性项间距

	QWidget* mContentWidget = nullptr;
	QVBoxLayout* mContentLayout = nullptr;

	// 分组管理
	int mNextGroupId = 1;
	QMap<int, DACollapsiblePanel*> mGroups;            // groupId -> collapsible panel wrapper
	QMap<int, DAPropertyPanelWidget*> mGroupPanels;    // groupId -> inner property panel
	DAPropertyPanelWidget* mCurrentGroupPanel = nullptr; // 当前活动分组面板（null=根面板）

	// 子面板管理
	QMap<int, DAPropertyPanelWidget*> mSubPanels;       // subPanelId -> sub-panel
	QMap<int, QMetaObject::Connection> mSubPanelConnections; // subPanelId -> signal connection

	QWidget* getTargetContentWidget() const;

	int generateAutoId();
	void addItemToContent(QWidget* widget, int index);
	void addItemToRoot(QWidget* widget, int index);
	void removeItemFromContent(QWidget* widget);
	int calculateAutoPropertyNameWidth() const;
	void updateAllItemsNameLabelWidth();
	DAPropertyItemWidget* createPropertyItem(int id,
	                                         const QString& name,
	                                         const QString& description,
	                                         QWidget* editor,
	                                         DAPropertyItemWidget::LayoutMode mode);
};

//===================================================
// DAPropertyPanelWidget::PrivateData
//===================================================

DAPropertyPanelWidget::PrivateData::PrivateData(DAPropertyPanelWidget* p) : q_ptr(p)
{
	if (p->objectName().isEmpty()) {
		p->setObjectName(QStringLiteral("DAPropertyPanelWidget"));
	}

	// 创建主布局
	QVBoxLayout* mainLayout = new QVBoxLayout(p);
	mainLayout->setObjectName(QStringLiteral("mainLayout"));
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	// 创建内容Widget（直接子控件，不再嵌入ScrollArea）
	mContentWidget = new QWidget();
	mContentWidget->setObjectName(QStringLiteral("contentWidget"));
	mContentLayout = new QVBoxLayout(mContentWidget);
	mContentLayout->setObjectName(QStringLiteral("contentLayout"));
	mContentLayout->setSpacing(mSpacing);
	mContentLayout->setContentsMargins(4, 4, 4, 4);

	// 底部弹性空间
	mContentLayout->addStretch(1);

	mainLayout->addWidget(mContentWidget);
}

QWidget* DAPropertyPanelWidget::PrivateData::getTargetContentWidget() const
{
	if (mCurrentGroupPanel) {
		// Access PrivateData of another DAPropertyPanelWidget instance (same class, class-level access)
		return mCurrentGroupPanel->d_func()->mContentWidget;
	}
	return mContentWidget;
}

int DAPropertyPanelWidget::PrivateData::generateAutoId()
{
	while (mPropertyItems.contains(mNextAutoId)) {
		++mNextAutoId;
	}
	return mNextAutoId++;
}

void DAPropertyPanelWidget::PrivateData::addItemToRoot(QWidget* widget, int index)
{
	int stretchIndex = mContentLayout->count() - 1;
	if (index < 0 || index >= stretchIndex) {
		mContentLayout->insertWidget(stretchIndex, widget);
	} else {
		mContentLayout->insertWidget(index, widget);
	}
}

void DAPropertyPanelWidget::PrivateData::removeItemFromContent(QWidget* widget)
{
	if (mCurrentGroupPanel) {
		mCurrentGroupPanel->d_func()->mContentLayout->removeWidget(widget);
	} else {
		mContentLayout->removeWidget(widget);
	}
}

void DAPropertyPanelWidget::PrivateData::addItemToContent(QWidget* widget, int index)
{
	int stretchIndex;
	QVBoxLayout* targetLayout;

	if (mCurrentGroupPanel) {
		targetLayout = mCurrentGroupPanel->d_func()->mContentLayout;
	} else {
		targetLayout = mContentLayout;
	}

	// stretch项始终在最后
	stretchIndex = targetLayout->count() - 1;
	if (index < 0 || index >= stretchIndex) {
		targetLayout->insertWidget(stretchIndex, widget);
	} else {
		targetLayout->insertWidget(index, widget);
	}
}

int DAPropertyPanelWidget::PrivateData::calculateAutoPropertyNameWidth() const
{
	int maxWidth = 60;
	QFontMetrics fm(mContentWidget->font());

	for (auto it = mPropertyItems.begin(); it != mPropertyItems.end(); ++it) {
		DAPropertyItemWidget* item = it.value();
		if (item->layoutMode() == DAPropertyItemWidget::InlineLayout) {
			QString name = item->propertyName();
			int w = Qt5Qt6Compat_fontMetrics_width(fm, name);
			maxWidth = qMax(maxWidth, w + 10);
		}
	}

	return qMin(maxWidth, 200);
}

void DAPropertyPanelWidget::PrivateData::updateAllItemsNameLabelWidth()
{
	int width = mPropertyNameWidth;
	if (width < 0) {
		width = calculateAutoPropertyNameWidth();
	}

	for (auto it = mPropertyItems.begin(); it != mPropertyItems.end(); ++it) {
		it.value()->setNameLabelWidth(width);
	}
}

DAPropertyItemWidget* DAPropertyPanelWidget::PrivateData::createPropertyItem(int id,
                                                                             const QString& name,
                                                                             const QString& description,
                                                                             QWidget* editor,
                                                                             DAPropertyItemWidget::LayoutMode mode)
{
	DAPropertyItemWidget* item = new DAPropertyItemWidget(q_ptr);
	item->setPropertyId(id);
	item->setPropertyName(name);
	item->setPropertyDescription(description);
	if (editor) {
		item->setEditorWidget(editor);
	}
	item->setLayoutMode(mode);
	item->setFrameVisible(false);

	int width = mPropertyNameWidth;
	if (width < 0) {
		width = calculateAutoPropertyNameWidth();
	}
	item->setNameLabelWidth(width);

	return item;
}

//===================================================
// DAPropertyPanelWidget
//===================================================

DAPropertyPanelWidget::DAPropertyPanelWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
}

DAPropertyPanelWidget::~DAPropertyPanelWidget()
{
}

// === 分组管理 ===

/**
 * @brief 添加可折叠分组，后续addXxxProperty自动添加到该分组
 * @param[in] title 分组标题
 * @return 分组ID（从1开始递增）
 */
int DAPropertyPanelWidget::addCollapsibleGroup(const QString& title)
{
	DA_D(d);
	// 创建折叠面板包装
	DACollapsiblePanel* groupPanel = new DACollapsiblePanel(title, d->mContentWidget);
	groupPanel->setObjectName(QStringLiteral("groupPanel_") + QString::number(d->mNextGroupId));

	// 创建分组内部属性面板作为内容控件
	DAPropertyPanelWidget* innerPanel = new DAPropertyPanelWidget(groupPanel);
	innerPanel->setObjectName(QStringLiteral("groupInnerPanel_") + QString::number(d->mNextGroupId));
	groupPanel->setContentWidget(innerPanel);

	int groupId = d->mNextGroupId++;

	// 连接折叠信号
	connect(groupPanel, &DACollapsiblePanel::expandedChanged, this, [this, groupId](bool expanded) {
		Q_UNUSED(expanded);
		// 可通过emit groupExpandedChanged(groupId, expanded)扩展
	});

	// 存储到分组映射
	d->mGroups[groupId] = groupPanel;
	d->mGroupPanels[groupId] = innerPanel;

	// 将折叠面板添加到根布局
	d->addItemToContent(groupPanel, -1);

	// 设置为当前活动分组
	d->mCurrentGroupPanel = innerPanel;

	return groupId;
}

/**
 * @brief 结束当前分组，后续addXxxProperty回到根面板
 */
void DAPropertyPanelWidget::endGroup()
{
	DA_D(d);
	d->mCurrentGroupPanel = nullptr;
}

/**
 * @brief 添加嵌套子面板（带ID映射和信号转发）
 * @param[in] id 子面板ID（由调用者指定）
 * @param[in] groupName 子面板标题
 * @return 子面板指针
 */
DAPropertyPanelWidget* DAPropertyPanelWidget::addSubPanel(int id, const QString& groupName)
{
	DA_D(d);
	// 创建折叠面板包装
	DACollapsiblePanel* subPanelWrapper = new DACollapsiblePanel(groupName, d->mContentWidget);
	subPanelWrapper->setObjectName(QStringLiteral("subPanelWrapper_") + QString::number(id));

	// 创建子面板作为内容控件
	DAPropertyPanelWidget* subPanel = new DAPropertyPanelWidget(subPanelWrapper);
	subPanel->setObjectName(QStringLiteral("subPanel_") + QString::number(id));
	subPanelWrapper->setContentWidget(subPanel);

	// 存储到子面板映射
	d->mSubPanels[id] = subPanel;

	// 将折叠面板添加到目标布局（有活动分组时进入分组，否则到根布局）
	d->addItemToContent(subPanelWrapper, -1);

	// 信号转发（子面板的属性变化自动冒泡到父面板）
	QMetaObject::Connection conn = connect(subPanel, &DAPropertyPanelWidget::propertyValueChanged,
	                                       this, &DAPropertyPanelWidget::propertyValueChanged);
	d->mSubPanelConnections[id] = conn;

	return subPanel;
}

/**
 * @brief 根据ID获取子面板
 * @param[in] id 子面板ID
 * @return 子面板指针，不存在则返回nullptr
 */
DAPropertyPanelWidget* DAPropertyPanelWidget::getSubPanel(int id) const
{
	DA_DC(d);
	return d->mSubPanels.value(id, nullptr);
}

/**
 * @brief 获取子面板对应的ID
 * @param[in] subPanel 子面板指针
 * @return 子面板ID，不存在则返回-1
 */
int DAPropertyPanelWidget::getSubPanelId(DAPropertyPanelWidget* subPanel) const
{
	DA_DC(d);
	return d->mSubPanels.key(subPanel, -1);
}

/**
 * @brief 获取分组面板指针
 * @param[in] groupId 分组ID
 * @return 分组面板指针（内部属性面板），不存在则返回nullptr
 */
DAPropertyPanelWidget* DAPropertyPanelWidget::getGroupPanel(int groupId) const
{
	DA_DC(d);
	return d->mGroupPanels.value(groupId, nullptr);
}

/**
 * @brief 获取分组展开状态
 * @param[in] groupId 分组ID
 * @return true为展开，false为收起
 */
bool DAPropertyPanelWidget::isGroupExpanded(int groupId) const
{
	DA_DC(d);
	DACollapsiblePanel* group = d->mGroups.value(groupId, nullptr);
	if (group) {
		return group->isExpanded();
	}
	return false;
}

/**
 * @brief 设置分组展开状态
 * @param[in] groupId 分组ID
 * @param[in] expanded true为展开，false为收起
 */
void DAPropertyPanelWidget::setGroupExpanded(int groupId, bool expanded)
{
	DA_D(d);
	DACollapsiblePanel* group = d->mGroups.value(groupId, nullptr);
	if (group) {
		group->setExpanded(expanded);
	}
}

// === 添加属性项（末尾添加）===

/**
 * @brief 获取当前目标面板（有活动分组时返回分组面板，否则返回根面板）
 */
DAPropertyPanelWidget* DAPropertyPanelWidget::getTargetPanel() const
{
	DA_DC(d);
	if (d->mCurrentGroupPanel) {
		return d->mCurrentGroupPanel;
	}
	return const_cast<DAPropertyPanelWidget*>(this);
}

/**
 * @brief 添加属性项（简化版，无description）
 *
 * @param[in] name 属性名称
 * @param[in] editor 编辑Widget
 * @param[in] mode 布局模式，默认Inline
 * @return 属性ID（自动分配）
 */
int DAPropertyPanelWidget::addProperty(const QString& name,
                                       QWidget* editor,
                                       DAPropertyItemWidget::LayoutMode mode)
{
	return addProperty(-1, name, QString(), editor, mode);
}

/**
 * @brief 添加属性项（指定ID，无description）
 *
 * @param[in] id 用户指定的属性ID
 * @param[in] name 属性名称
 * @param[in] editor 编辑Widget
 * @param[in] mode 布局模式，默认Inline
 * @return 属性ID（返回传入的id）
 */
int DAPropertyPanelWidget::addProperty(int id,
                                       const QString& name,
                                       QWidget* editor,
                                       DAPropertyItemWidget::LayoutMode mode)
{
	return addProperty(id, name, QString(), editor, mode);
}

/**
 * @brief 添加属性项（完整参数）
 *
 * @param[in] name 属性名称
 * @param[in] description 属性描述（tooltip）
 * @param[in] editor 编辑Widget
 * @param[in] mode 布局模式，默认Inline
 * @return 属性ID（自动分配）
 */
int DAPropertyPanelWidget::addProperty(const QString& name,
                                       const QString& description,
                                       QWidget* editor,
                                       DAPropertyItemWidget::LayoutMode mode)
{
	return addProperty(-1, name, description, editor, mode);
}

/**
 * @brief 添加属性项（完整参数，指定ID）
 *
 * @param[in] id 用户指定的属性ID，-1表示自动分配
 * @param[in] name 属性名称
 * @param[in] description 属性描述（tooltip）
 * @param[in] editor 编辑Widget
 * @param[in] mode 布局模式，默认Inline
 * @return 属性ID（返回传入的id或自动分配的id）
 */
int DAPropertyPanelWidget::addProperty(int id,
                                       const QString& name,
                                       const QString& description,
                                       QWidget* editor,
                                       DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	// 如果有活动分组，路由到分组面板
	if (d->mCurrentGroupPanel) {
		return d->mCurrentGroupPanel->addProperty(id, name, description, editor, mode);
	}

	// 根面板逻辑（保持原有行为）
	if (id < 0) {
		id = d->generateAutoId();
	}
	if (d->mPropertyItems.contains(id)) {
		return -1;
	}

	DAPropertyItemWidget* item = d->createPropertyItem(id, name, description, editor, mode);
	d->mPropertyItems[id] = item;
	d->mWidgetList.append(item);
	d->addItemToContent(item, -1);
	connectItemSignals(item);

	if (d->mPropertyNameWidth < 0 && mode == DAPropertyItemWidget::InlineLayout) {
		d->updateAllItemsNameLabelWidth();
	}

	return id;
}

// === 插入属性项（指定位置）===

/**
 * @brief 在指定索引位置插入属性项
 *
 * @param[in] index 插入位置索引，-1表示末尾
 * @param[in] name 属性名称
 * @param[in] editor 编辑Widget
 * @param[in] mode 布局模式
 * @return 属性ID
 */
int DAPropertyPanelWidget::insertProperty(int index,
                                          const QString& name,
                                          QWidget* editor,
                                          DAPropertyItemWidget::LayoutMode mode)
{
	return insertProperty(index, -1, name, QString(), editor, mode);
}

/**
 * @brief 在指定索引位置插入属性项（指定ID）
 */
int DAPropertyPanelWidget::insertProperty(int index,
                                          int id,
                                          const QString& name,
                                          QWidget* editor,
                                          DAPropertyItemWidget::LayoutMode mode)
{
	return insertProperty(index, id, name, QString(), editor, mode);
}

/**
 * @brief 在指定索引位置插入属性项（完整参数）
 */
int DAPropertyPanelWidget::insertProperty(int index,
                                          const QString& name,
                                          const QString& description,
                                          QWidget* editor,
                                          DAPropertyItemWidget::LayoutMode mode)
{
	return insertProperty(index, -1, name, description, editor, mode);
}

/**
 * @brief 在指定索引位置插入属性项（完整参数，指定ID）
 *
 * @param[in] index 插入位置索引，-1表示末尾
 * @param[in] id 属性ID，-1表示自动分配
 * @param[in] name 属性名称
 * @param[in] description 属性描述
 * @param[in] editor 编辑Widget
 * @param[in] mode 布局模式
 * @return 属性ID
 */
int DAPropertyPanelWidget::insertProperty(int index,
                                          int id,
                                          const QString& name,
                                          const QString& description,
                                          QWidget* editor,
                                          DAPropertyItemWidget::LayoutMode mode)
{
	DA_D(d);
	// 如果有活动分组，路由到分组面板
	if (d->mCurrentGroupPanel) {
		return d->mCurrentGroupPanel->insertProperty(index, id, name, description, editor, mode);
	}

	// 根面板逻辑（保持原有行为）
	if (id < 0) {
		id = d->generateAutoId();
	}
	if (d->mPropertyItems.contains(id)) {
		return -1;
	}

	DAPropertyItemWidget* item = d->createPropertyItem(id, name, description, editor, mode);
	d->mPropertyItems[id] = item;

	if (index < 0 || index >= d->mWidgetList.size()) {
		d->mWidgetList.append(item);
	} else {
		d->mWidgetList.insert(index, item);
	}
	d->addItemToContent(item, index);
	connectItemSignals(item);

	if (d->mPropertyNameWidth < 0 && mode == DAPropertyItemWidget::InlineLayout) {
		d->updateAllItemsNameLabelWidth();
	}

	return id;
}

// === 分隔项 ===

/**
 * @brief 添加空白间距分隔（末尾）— 始终添加到根布局
 *
 * @param[in] height 间距高度（像素），默认8
 */
void DAPropertyPanelWidget::addSpacer(int height)
{
	DA_D(d);
	QWidget* spacerWidget = new QWidget(d->mContentWidget);
	spacerWidget->setObjectName(QStringLiteral("spacerWidget"));
	spacerWidget->setFixedHeight(height);
	// 始终添加到根布局
	d->addItemToRoot(spacerWidget, -1);
	d->mWidgetList.append(spacerWidget);
}

/**
 * @brief 在指定位置插入空白间距分隔
 */
void DAPropertyPanelWidget::insertSpacer(int index, int height)
{
	DA_D(d);
	QWidget* spacerWidget = new QWidget(d->mContentWidget);
	spacerWidget->setFixedHeight(height);
	spacerWidget->setObjectName(QStringLiteral("spacerWidget"));
	if (index < 0 || index >= d->mWidgetList.size()) {
		d->mWidgetList.append(spacerWidget);
	} else {
		d->mWidgetList.insert(index, spacerWidget);
	}
	// 始终添加到根布局
	d->addItemToRoot(spacerWidget, index);
}

/**
 * @brief 添加水平线分隔（末尾）— 始终添加到根布局
 */
void DAPropertyPanelWidget::addSeparator()
{
	insertSeparator(-1);
}

/**
 * @brief 在指定位置插入水平线分隔
 */
void DAPropertyPanelWidget::insertSeparator(int index)
{
	DA_D(d);
	QFrame* line = new QFrame(d->mContentWidget);
	line->setObjectName(QStringLiteral("separatorLine"));
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	line->setFixedHeight(2);
	if (index < 0 || index >= d->mWidgetList.size()) {
		d->mWidgetList.append(line);
	} else {
		d->mWidgetList.insert(index, line);
	}
	// 始终添加到根布局
	d->addItemToRoot(line, index);
}

// === 属性项管理 ===

/**
 * @brief 移除属性项
 *
 * @param[in] id 属性ID
 */
void DAPropertyPanelWidget::removeProperty(int id)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.take(id);
	if (item) {
		d->mWidgetList.removeOne(item);
		d->removeItemFromContent(item);
		item->deleteLater();

		if (d->mPropertyNameWidth < 0) {
			d->updateAllItemsNameLabelWidth();
		}
	}
}

/**
 * @brief 清除所有属性项
 */
void DAPropertyPanelWidget::clearProperties()
{
	DA_D(d);
	// 清除所有widget
	for (QWidget* w : d->mWidgetList) {
		d->mContentLayout->removeWidget(w);
		w->deleteLater();
	}
	d->mWidgetList.clear();
	d->mPropertyItems.clear();

	// 清除分组映射
	d->mGroups.clear();
	d->mGroupPanels.clear();
	d->mCurrentGroupPanel = nullptr;

	// 清除子面板及信号连接
	for (auto it = d->mSubPanelConnections.begin(); it != d->mSubPanelConnections.end(); ++it) {
		disconnect(it.value());
	}
	d->mSubPanelConnections.clear();
	d->mSubPanels.clear();
}

/**
 * @brief 获取属性项Widget（通过ID）
 */
DAPropertyItemWidget* DAPropertyPanelWidget::getPropertyItem(int id) const
{
	DA_DC(d);
	return d->mPropertyItems.value(id, nullptr);
}

/**
 * @brief 获取属性项Widget（通过索引）
 */
DAPropertyItemWidget* DAPropertyPanelWidget::getPropertyItemAt(int index) const
{
	DA_DC(d);
	if (index < 0 || index >= d->mWidgetList.size()) {
		return nullptr;
	}
	// 需要从mWidgetList中筛选出DAPropertyItemWidget
	int propIndex = 0;
	for (QWidget* w : d->mWidgetList) {
		DAPropertyItemWidget* item = qobject_cast<DAPropertyItemWidget*>(w);
		if (item) {
			if (propIndex == index) {
				return item;
			}
			++propIndex;
		}
	}
	return nullptr;
}

/**
 * @brief 获取属性项的索引位置
 */
int DAPropertyPanelWidget::indexOf(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id);
	if (!item) {
		return -1;
	}
	int index = d->mWidgetList.indexOf(item);
	return index;
}

/**
 * @brief 获取所有属性项ID列表
 */
QList<int> DAPropertyPanelWidget::propertyIds() const
{
	DA_DC(d);
	return d->mPropertyItems.keys();
}

/**
 * @brief 属性项数量
 */
int DAPropertyPanelWidget::propertyCount() const
{
	DA_DC(d);
	return d->mPropertyItems.size();
}

// === 遍历接口 ===

/**
 * @brief 遍历所有属性项
 *
 * @param[in] callback 遍历回调函数，返回false停止遍历
 */
void DAPropertyPanelWidget::traverseItems(TraverseCallback callback)
{
	DA_D(d);
	for (QWidget* w : d->mWidgetList) {
		DAPropertyItemWidget* item = qobject_cast<DAPropertyItemWidget*>(w);
		if (item) {
			if (!callback(item)) {
				break;
			}
		}
	}
}

/**
 * @brief 遍历所有属性项（只读）
 */
void DAPropertyPanelWidget::traverseItems(TraverseCallback callback) const
{
	DA_DC(d);
	for (QWidget* w : d->mWidgetList) {
		DAPropertyItemWidget* item = qobject_cast<DAPropertyItemWidget*>(w);
		if (item) {
			if (!callback(item)) {
				break;
			}
		}
	}
}

/**
 * @brief 获取所有属性项Widget列表
 */
DAPropertyPanelWidget::PropertyItemList DAPropertyPanelWidget::allPropertyItems() const
{
	DA_DC(d);
	PropertyItemList result;
	for (QWidget* w : d->mWidgetList) {
		DAPropertyItemWidget* item = qobject_cast<DAPropertyItemWidget*>(w);
		if (item) {
			result.append(item);
		}
	}
	return result;
}

// === 面板配置 ===

/**
 * @brief 设置属性名标签宽度（Inline模式）
 *
 * @param[in] width 宽度（像素），设为-1则自动计算
 */
void DAPropertyPanelWidget::setPropertyNameWidth(int width)
{
	DA_D(d);
	d->mPropertyNameWidth = width;
	d->updateAllItemsNameLabelWidth();
}

/**
 * @brief 获取属性名标签宽度
 */
int DAPropertyPanelWidget::propertyNameWidth() const
{
	DA_DC(d);
	if (d->mPropertyNameWidth < 0) {
		return d->calculateAutoPropertyNameWidth();
	}
	return d->mPropertyNameWidth;
}

/**
 * @brief 设置属性项间距
 */
void DAPropertyPanelWidget::setSpacing(int spacing)
{
	DA_D(d);
	d->mSpacing = spacing;
	if (d->mContentLayout) {
		d->mContentLayout->setSpacing(spacing);
	}
}

/**
 * @brief 获取属性项间距
 */
int DAPropertyPanelWidget::spacing() const
{
	DA_DC(d);
	return d->mSpacing;
}

/**
 * @brief 重新计算属性名标签宽度
 */
void DAPropertyPanelWidget::recalculatePropertyNameWidth()
{
	DA_D(d);
	d->updateAllItemsNameLabelWidth();
}

void DAPropertyPanelWidget::onItemValueChanged(int propertyId)
{
	emit propertyValueChanged(propertyId);
}

void DAPropertyPanelWidget::connectItemSignals(DAPropertyItemWidget* item)
{
	// 注意：DAPropertyItemWidget::valueChanged 信号当前未被触发。
	// 便捷属性方法（addColorProperty等）通过 lambda 直接连接编辑器 Widget 的信号到 propertyValueChanged，
	// 这是当前唯一的信号转发路径。如果未来 DAPropertyItemWidget 自身开始触发 valueChanged，
	// 需要同时移除便捷方法中的 lambda 连接以避免双重发射。
	connect(item, &DAPropertyItemWidget::valueChanged, this, &DAPropertyPanelWidget::onItemValueChanged);
}

// === 便捷属性添加方法 ===

/**
 * @brief 添加颜色属性
 */
int DAPropertyPanelWidget::addColorProperty(int id, const QString& name, const QColor& color)
{
	DA_D(d);
	DAColorPickerButton* btn = new DAColorPickerButton(d->getTargetContentWidget());
	btn->setColor(color);
	// 路由到目标面板（有分组时添加到分组，无分组时添加到根）
	int propId = addProperty(id, name, btn);
	// DAColorPickerButton 继承自 SAColorToolButton，colorChanged 信号来自基类
	connect(btn, &SAColorToolButton::colorChanged, this, [this, propId](const QColor&) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addColorProperty(const QString& name, const QColor& color)
{
	return addColorProperty(-1, name, color);
}

/**
 * @brief 添加字体属性
 */
int DAPropertyPanelWidget::addFontProperty(int id, const QString& name, const QFont& font)
{
	DA_D(d);
	DAFontEditPannelWidget* editor = new DAFontEditPannelWidget(d->getTargetContentWidget());
	editor->setCurrentFont(font);
	// 路由到目标面板
	int propId = addProperty(id, name, editor, DAPropertyItemWidget::BelowLayout);
	connect(editor, &DAFontEditPannelWidget::currentFontChanged, this, [this, propId](const QFont&) {
		emit propertyValueChanged(propId);
	});
	connect(editor, &DAFontEditPannelWidget::currentFontColorChanged, this, [this, propId](const QColor&) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addFontProperty(const QString& name, const QFont& font)
{
	return addFontProperty(-1, name, font);
}

/**
 * @brief 添加画刷属性
 */
int DAPropertyPanelWidget::addBrushProperty(int id, const QString& name, const QBrush& brush)
{
	DA_D(d);
	DABrushEditWidget* editor = new DABrushEditWidget(d->getTargetContentWidget());
	editor->setCurrentBrush(brush);
	// 路由到目标面板
	int propId = addProperty(id, name, editor, DAPropertyItemWidget::BelowLayout);
	connect(editor, &DABrushEditWidget::brushChanged, this, [this, propId](const QBrush&) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addBrushProperty(const QString& name, const QBrush& brush)
{
	return addBrushProperty(-1, name, brush);
}

/**
 * @brief 添加画笔属性
 */
int DAPropertyPanelWidget::addPenProperty(int id, const QString& name, const QPen& pen)
{
	DA_D(d);
	DAPenEditWidget* editor = new DAPenEditWidget(d->getTargetContentWidget());
	editor->setCurrentPen(pen);
	// 路由到目标面板
	int propId = addProperty(id, name, editor, DAPropertyItemWidget::BelowLayout);
	connect(editor, &DAPenEditWidget::penChanged, this, [this, propId](const QPen&) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addPenProperty(const QString& name, const QPen& pen)
{
	return addPenProperty(-1, name, pen);
}

/**
 * @brief 添加整数属性
 */
int DAPropertyPanelWidget::addIntProperty(int id, const QString& name, int value, int min, int max)
{
	DA_D(d);
	QSpinBox* spin = new QSpinBox(d->getTargetContentWidget());
	spin->setRange(min, max);
	spin->setValue(value);
	// 路由到目标面板
	int propId = addProperty(id, name, spin);
	connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, propId](int) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addIntProperty(const QString& name, int value, int min, int max)
{
	return addIntProperty(-1, name, value, min, max);
}

/**
 * @brief 添加浮点属性
 */
int DAPropertyPanelWidget::addDoubleProperty(int id, const QString& name, double value, double min, double max, int decimals)
{
	DA_D(d);
	QDoubleSpinBox* spin = new QDoubleSpinBox(d->getTargetContentWidget());
	spin->setRange(min, max);
	spin->setValue(value);
	spin->setDecimals(decimals);
	// 路由到目标面板
	int propId = addProperty(id, name, spin);
	connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, propId](double) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addDoubleProperty(const QString& name, double value, double min, double max, int decimals)
{
	return addDoubleProperty(-1, name, value, min, max, decimals);
}

/**
 * @brief 添加布尔属性
 */
int DAPropertyPanelWidget::addBoolProperty(int id, const QString& name, bool checked)
{
	DA_D(d);
	QCheckBox* checkBox = new QCheckBox(d->getTargetContentWidget());
	checkBox->setChecked(checked);
	// 路由到目标面板
	int propId = addProperty(id, name, checkBox);
	connect(checkBox, &QCheckBox::toggled, this, [this, propId](bool) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addBoolProperty(const QString& name, bool checked)
{
	return addBoolProperty(-1, name, checked);
}

/**
 * @brief 添加字符串属性
 */
int DAPropertyPanelWidget::addStringProperty(int id, const QString& name, const QString& text)
{
	DA_D(d);
	QLineEdit* editor = new QLineEdit(d->getTargetContentWidget());
	editor->setText(text);
	// 路由到目标面板
	int propId = addProperty(id, name, editor);
	connect(editor, &QLineEdit::textEdited, this, [this, propId](const QString&) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addStringProperty(const QString& name, const QString& text)
{
	return addStringProperty(-1, name, text);
}

/**
 * @brief 添加枚举属性（下拉框）
 */
int DAPropertyPanelWidget::addEnumProperty(int id, const QString& name, const QStringList& items, const QList<int>& dataValues, int currentIndex)
{
	DA_D(d);
	QComboBox* combo = new QComboBox(d->getTargetContentWidget());
	combo->addItems(items);
	// 设置 item data
	for (int i = 0; i < items.size(); ++i) {
		int dataVal = (i < dataValues.size()) ? dataValues[i] : i;
		combo->setItemData(i, dataVal);
	}
	if (currentIndex >= 0 && currentIndex < items.size()) {
		combo->setCurrentIndex(currentIndex);
	}
	// 路由到目标面板
        int propId = addProperty(id, name, combo);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, propId](int) {
            Q_EMIT propertyValueChanged(propId);
        });
#else
	connect(combo, &QComboBox::currentIndexChanged, this, [this, propId](int) {
                Q_EMIT propertyValueChanged(propId);
	});
#endif
	return propId;
}

int DAPropertyPanelWidget::addEnumProperty(const QString& name, const QStringList& items, const QList<int>& dataValues, int currentIndex)
{
	return addEnumProperty(-1, name, items, dataValues, currentIndex);
}

/**
 * @brief 添加对齐属性
 */
int DAPropertyPanelWidget::addAlignmentProperty(int id, const QString& name, Qt::Alignment alignment)
{
	DA_D(d);
	DAAligmentEditWidget* editor = new DAAligmentEditWidget(d->getTargetContentWidget());
	editor->setCurrentAlignment(alignment);
	// 路由到目标面板
	int propId = addProperty(id, name, editor);
	connect(editor, &DAAligmentEditWidget::alignmentChanged, this, [this, propId](Qt::Alignment) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addAlignmentProperty(const QString& name, Qt::Alignment alignment)
{
	return addAlignmentProperty(-1, name, alignment);
}

/**
 * @brief 添加对齐位置属性
 */
int DAPropertyPanelWidget::addAlignmentPositionProperty(int id, const QString& name, Qt::Alignment alignment)
{
	DA_D(d);
	DAAligmentPositionEditWidget* editor = new DAAligmentPositionEditWidget(d->getTargetContentWidget());
	editor->setAligmentPosition(alignment);
	// 路由到目标面板
	int propId = addProperty(id, name, editor);
	connect(editor, &DAAligmentPositionEditWidget::aligmentPositionChanged, this, [this, propId](Qt::Alignment) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addAlignmentPositionProperty(const QString& name, Qt::Alignment alignment)
{
	return addAlignmentPositionProperty(-1, name, alignment);
}

/**
 * @brief 添加文件路径属性
 */
int DAPropertyPanelWidget::addFilePathProperty(int id, const QString& name, const QString& filter)
{
	DA_D(d);
	DAFilePathEditWidget* editor = new DAFilePathEditWidget(d->getTargetContentWidget());
	if (!filter.isEmpty()) {
		editor->setNameFilter(filter);
	}
	// 路由到目标面板
	int propId = addProperty(id, name, editor);
	connect(editor, &DAFilePathEditWidget::selectedPath, this, [this, propId](const QString&) {
		emit propertyValueChanged(propId);
	});
	return propId;
}

int DAPropertyPanelWidget::addFilePathProperty(const QString& name, const QString& filter)
{
	return addFilePathProperty(-1, name, filter);
}

// === 分组标签 ===

/**
 * @brief 添加分组标签
 * @deprecated 建议使用addCollapsibleGroup代替，此方法仅创建装饰性标签不具备折叠功能
 */
void DAPropertyPanelWidget::addGroupLabel(const QString& text)
{
	DA_D(d);
	QLabel* label = new QLabel(text, d->mContentWidget);
	label->setObjectName(QStringLiteral("groupLabel"));
	QFont font = label->font();
	font.setBold(true);
	label->setFont(font);
	label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	label->setContentsMargins(0, 8, 0, 4);
	// 始终添加到根布局
	d->addItemToRoot(label, -1);
	d->mWidgetList.append(label);
}

/**
 * @brief 在指定位置插入分组标签
 */
void DAPropertyPanelWidget::insertGroupLabel(int index, const QString& text)
{
	DA_D(d);
	QLabel* label = new QLabel(text, d->mContentWidget);
	label->setObjectName(QStringLiteral("groupLabel"));
	QFont font = label->font();
	font.setBold(true);
	label->setFont(font);
	label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	label->setContentsMargins(0, 8, 0, 4);
	if (index < 0 || index >= d->mWidgetList.size()) {
		d->mWidgetList.append(label);
	} else {
		d->mWidgetList.insert(index, label);
	}
	// 始终添加到根布局
	d->addItemToRoot(label, index);
}

// === 值读写方法 ===

/**
 * @brief 获取颜色值
 */
QColor DAPropertyPanelWidget::getColorValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return QColor();
	QWidget* editor = item->editorWidget();
	if (!editor) return QColor();
	DAColorPickerButton* btn = qobject_cast<DAColorPickerButton*>(editor);
	if (btn) return btn->color();
	return QColor();
}

/**
 * @brief 设置颜色值
 */
void DAPropertyPanelWidget::setColorValue(int id, const QColor& color)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	DAColorPickerButton* btn = qobject_cast<DAColorPickerButton*>(editor);
	if (btn) {
		QSignalBlocker blocker(btn);
		btn->setColor(color);
	}
}

/**
 * @brief 获取字体值
 */
QFont DAPropertyPanelWidget::getFontValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return QFont();
	QWidget* editor = item->editorWidget();
	if (!editor) return QFont();
	DAFontEditPannelWidget* w = qobject_cast<DAFontEditPannelWidget*>(editor);
	if (w) return w->getCurrentFont();
	return QFont();
}

/**
 * @brief 设置字体值
 */
void DAPropertyPanelWidget::setFontValue(int id, const QFont& font)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	DAFontEditPannelWidget* w = qobject_cast<DAFontEditPannelWidget*>(editor);
	if (w) {
		QSignalBlocker blocker(w);
		w->setCurrentFont(font);
	}
}

/**
 * @brief 获取画刷值
 */
QBrush DAPropertyPanelWidget::getBrushValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return QBrush();
	QWidget* editor = item->editorWidget();
	if (!editor) return QBrush();
	DABrushEditWidget* w = qobject_cast<DABrushEditWidget*>(editor);
	if (w) return w->getCurrentBrush();
	return QBrush();
}

/**
 * @brief 设置画刷值
 */
void DAPropertyPanelWidget::setBrushValue(int id, const QBrush& brush)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	DABrushEditWidget* w = qobject_cast<DABrushEditWidget*>(editor);
	if (w) {
		QSignalBlocker blocker(w);
		w->setCurrentBrush(brush);
	}
}

/**
 * @brief 获取画笔值
 */
QPen DAPropertyPanelWidget::getPenValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return QPen();
	QWidget* editor = item->editorWidget();
	if (!editor) return QPen();
	DAPenEditWidget* w = qobject_cast<DAPenEditWidget*>(editor);
	if (w) return w->getCurrentPen();
	return QPen();
}

/**
 * @brief 设置画笔值
 */
void DAPropertyPanelWidget::setPenValue(int id, const QPen& pen)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	DAPenEditWidget* w = qobject_cast<DAPenEditWidget*>(editor);
	if (w) {
		QSignalBlocker blocker(w);
		w->setCurrentPen(pen);
	}
}

/**
 * @brief 获取整数值
 */
int DAPropertyPanelWidget::getIntValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return 0;
	QWidget* editor = item->editorWidget();
	if (!editor) return 0;
	QSpinBox* spin = qobject_cast<QSpinBox*>(editor);
	if (spin) return spin->value();
	return 0;
}

/**
 * @brief 设置整数值
 */
void DAPropertyPanelWidget::setIntValue(int id, int value)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	QSpinBox* spin = qobject_cast<QSpinBox*>(editor);
	if (spin) {
		QSignalBlocker blocker(spin);
		spin->setValue(value);
	}
}

/**
 * @brief 获取浮点值
 */
double DAPropertyPanelWidget::getDoubleValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return 0.0;
	QWidget* editor = item->editorWidget();
	if (!editor) return 0.0;
	QDoubleSpinBox* spin = qobject_cast<QDoubleSpinBox*>(editor);
	if (spin) return spin->value();
	return 0.0;
}

/**
 * @brief 设置浮点值
 */
void DAPropertyPanelWidget::setDoubleValue(int id, double value)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	QDoubleSpinBox* spin = qobject_cast<QDoubleSpinBox*>(editor);
	if (spin) {
		QSignalBlocker blocker(spin);
		spin->setValue(value);
	}
}

/**
 * @brief 获取布尔值
 */
bool DAPropertyPanelWidget::getBoolValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return false;
	QWidget* editor = item->editorWidget();
	if (!editor) return false;
	QCheckBox* cb = qobject_cast<QCheckBox*>(editor);
	if (cb) return cb->isChecked();
	return false;
}

/**
 * @brief 设置布尔值
 */
void DAPropertyPanelWidget::setBoolValue(int id, bool checked)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	QCheckBox* cb = qobject_cast<QCheckBox*>(editor);
	if (cb) {
		QSignalBlocker blocker(cb);
		cb->setChecked(checked);
	}
}

/**
 * @brief 获取字符串值
 */
QString DAPropertyPanelWidget::getStringValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return QString();
	QWidget* editor = item->editorWidget();
	if (!editor) return QString();
	QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
	if (edit) return edit->text();
	return QString();
}

/**
 * @brief 设置字符串值
 */
void DAPropertyPanelWidget::setStringValue(int id, const QString& text)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
	if (edit) {
		QSignalBlocker blocker(edit);
		edit->setText(text);
	}
}

/**
 * @brief 获取枚举值（如果设置了itemData，返回data值；否则返回索引）
 */
int DAPropertyPanelWidget::getEnumValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return -1;
	QWidget* editor = item->editorWidget();
	if (!editor) return -1;
	QComboBox* combo = qobject_cast<QComboBox*>(editor);
	if (combo) {
		QVariant data = combo->currentData();
		if (data.isValid()) {
			return data.toInt();
		}
		return combo->currentIndex();
	}
	return -1;
}

/**
 * @brief 设置枚举值（如果设置了itemData，按data值查找；否则按索引设置）
 */
void DAPropertyPanelWidget::setEnumValue(int id, int value)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	QComboBox* combo = qobject_cast<QComboBox*>(editor);
	if (combo) {
		QSignalBlocker blocker(combo);
		QVariant data = combo->itemData(0);
		if (data.isValid()) {
			int idx = combo->findData(value);
			if (idx >= 0) {
				combo->setCurrentIndex(idx);
			}
		} else {
			combo->setCurrentIndex(value);
		}
	}
}

/**
 * @brief 获取对齐值
 */
Qt::Alignment DAPropertyPanelWidget::getAlignmentValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return Qt::Alignment();
	QWidget* editor = item->editorWidget();
	if (!editor) return Qt::Alignment();
	DAAligmentEditWidget* w = qobject_cast<DAAligmentEditWidget*>(editor);
	if (w) return w->getCurrentAlignment();
	return Qt::Alignment();
}

/**
 * @brief 设置对齐值
 */
void DAPropertyPanelWidget::setAlignmentValue(int id, Qt::Alignment alignment)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	DAAligmentEditWidget* w = qobject_cast<DAAligmentEditWidget*>(editor);
	if (w) {
		QSignalBlocker blocker(w);
		w->setCurrentAlignment(alignment);
	}
}

/**
 * @brief 获取对齐位置值
 */
Qt::Alignment DAPropertyPanelWidget::getAlignmentPositionValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return Qt::Alignment();
	QWidget* editor = item->editorWidget();
	if (!editor) return Qt::Alignment();
	DAAligmentPositionEditWidget* w = qobject_cast<DAAligmentPositionEditWidget*>(editor);
	if (w) return w->getAligmentPosition();
	return Qt::Alignment();
}

/**
 * @brief 设置对齐位置值
 */
void DAPropertyPanelWidget::setAlignmentPositionValue(int id, Qt::Alignment alignment)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	DAAligmentPositionEditWidget* w = qobject_cast<DAAligmentPositionEditWidget*>(editor);
	if (w) {
		QSignalBlocker blocker(w);
		w->setAligmentPosition(alignment);
	}
}

/**
 * @brief 获取文件路径值
 */
QString DAPropertyPanelWidget::getFilePathValue(int id) const
{
	DA_DC(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return QString();
	QWidget* editor = item->editorWidget();
	if (!editor) return QString();
	DAFilePathEditWidget* w = qobject_cast<DAFilePathEditWidget*>(editor);
	if (w) return w->getFilePath();
	return QString();
}

/**
 * @brief 设置文件路径值
 */
void DAPropertyPanelWidget::setFilePathValue(int id, const QString& path)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (!item) return;
	QWidget* editor = item->editorWidget();
	if (!editor) return;
	DAFilePathEditWidget* w = qobject_cast<DAFilePathEditWidget*>(editor);
	if (w) {
		QSignalBlocker blocker(w);
		w->setFilePath(path);
	}
}

// === 可见性与状态控制 ===

/**
 * @brief 设置属性可见性
 */
void DAPropertyPanelWidget::setPropertyVisible(int id, bool visible)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (item) {
		if (visible)
			item->show();
		else
			item->hide();
	}
}

/**
 * @brief 设置属性启用状态
 */
void DAPropertyPanelWidget::setPropertyEnabled(int id, bool enabled)
{
	DA_D(d);
	DAPropertyItemWidget* item = d->mPropertyItems.value(id, nullptr);
	if (item) {
		item->setEnabled(enabled);
	}
}

/**
 * @brief 检查属性是否存在
 */
bool DAPropertyPanelWidget::propertyExists(int id) const
{
	DA_DC(d);
	return d->mPropertyItems.contains(id);
}

}  // namespace DA
