#include "DAPropertyPanelWidget.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFrame>
#include <QFontMetrics>
#include <QSpacerItem>
#include "DAGlobals.h"

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

	QScrollArea* mScrollArea = nullptr;
	QWidget* mContentWidget = nullptr;
	QVBoxLayout* mContentLayout = nullptr;

	int generateAutoId();
	void addItemToContent(QWidget* widget, int index);
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

	// 创建ScrollArea
	mScrollArea = new QScrollArea(p);
	mScrollArea->setObjectName(QStringLiteral("scrollArea"));
	mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mScrollArea->setWidgetResizable(true);
	mScrollArea->setFrameShape(QFrame::NoFrame);

	// 创建内容Widget
	mContentWidget = new QWidget();
	mContentWidget->setObjectName(QStringLiteral("contentWidget"));
	mContentLayout = new QVBoxLayout(mContentWidget);
	mContentLayout->setObjectName(QStringLiteral("contentLayout"));
	mContentLayout->setSpacing(mSpacing);
	mContentLayout->setContentsMargins(4, 4, 4, 4);

	// 底部弹性空间
	mContentLayout->addStretch(1);

	mScrollArea->setWidget(mContentWidget);

	// 将ScrollArea设为DAPropertyPanelWidget的主布局
	QVBoxLayout* mainLayout = new QVBoxLayout(p);
	mainLayout->setObjectName(QStringLiteral("mainLayout"));
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(mScrollArea);
}

int DAPropertyPanelWidget::PrivateData::generateAutoId()
{
	while (mPropertyItems.contains(mNextAutoId)) {
		++mNextAutoId;
	}
	return mNextAutoId++;
}

void DAPropertyPanelWidget::PrivateData::addItemToContent(QWidget* widget, int index)
{
	// stretch项始终在最后，所以插入位置需要在stretch之前调整
	int stretchIndex = mContentLayout->count() - 1;  // stretch是最后一个项
	if (index < 0 || index >= stretchIndex) {
		// 插入到stretch之前（即末尾）
		mContentLayout->insertWidget(stretchIndex, widget);
	} else {
		mContentLayout->insertWidget(index, widget);
	}
}

void DAPropertyPanelWidget::PrivateData::removeItemFromContent(QWidget* widget)
{
	mContentLayout->removeWidget(widget);
}

int DAPropertyPanelWidget::PrivateData::calculateAutoPropertyNameWidth() const
{
	int maxWidth = 60;  // 最小宽度
	QFontMetrics fm(mContentWidget->font());

	for (auto it = mPropertyItems.begin(); it != mPropertyItems.end(); ++it) {
		DAPropertyItemWidget* item = it.value();
		if (item->layoutMode() == DAPropertyItemWidget::InlineLayout) {
			QString name = item->propertyName();
			int w = Qt5Qt6Compat_fontMetrics_width(fm, name);
			maxWidth = qMax(maxWidth, w + 10);  // 加10px边距
		}
	}

	return qMin(maxWidth, 200);  // 最大200px
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

	// 设置属性名宽度
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

// === 添加属性项（末尾添加）===

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
	// 如果id为-1，自动分配
	if (id < 0) {
		id = d->generateAutoId();
	}
	// 检查ID是否已存在
	if (d->mPropertyItems.contains(id)) {
		return -1;  // ID冲突
	}

	DAPropertyItemWidget* item = d->createPropertyItem(id, name, description, editor, mode);
	d->mPropertyItems[id] = item;
	d->mWidgetList.append(item);
	d->addItemToContent(item, -1);
	connectItemSignals(item);

	// 自动计算宽度可能需要更新
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
	if (id < 0) {
		id = d->generateAutoId();
	}
	if (d->mPropertyItems.contains(id)) {
		return -1;
	}

	DAPropertyItemWidget* item = d->createPropertyItem(id, name, description, editor, mode);
	d->mPropertyItems[id] = item;

	// 插入到指定位置
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
 * @brief 添加空白间距分隔（末尾）
 * 
 * @param[in] height 间距高度（像素），默认8
 */
void DAPropertyPanelWidget::addSpacer(int height)
{
	DA_D(d);
	QSpacerItem* spacer = new QSpacerItem(0, height, QSizePolicy::Minimum, QSizePolicy::Fixed);
	int stretchIndex = d->mContentLayout->count() - 1;
	d->mContentLayout->insertItem(stretchIndex, spacer);
	// 记录一个占位widget用于索引计算
	QWidget* spacerWidget = new QWidget(d->mContentWidget);
	spacerWidget->setFixedHeight(height);
	spacerWidget->setObjectName(QStringLiteral("spacerWidget"));
	d->mWidgetList.append(spacerWidget);
	d->addItemToContent(spacerWidget, -1);
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
	d->addItemToContent(spacerWidget, index);
}

/**
 * @brief 添加水平线分隔（末尾）
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
	d->addItemToContent(line, index);
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

		// 重新计算宽度
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

	// 重新计算宽度
	if (d->mPropertyNameWidth < 0) {
		d->updateAllItemsNameLabelWidth();
	}
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
	connect(item, &DAPropertyItemWidget::valueChanged, this, &DAPropertyPanelWidget::onItemValueChanged);
}

}  // namespace DA