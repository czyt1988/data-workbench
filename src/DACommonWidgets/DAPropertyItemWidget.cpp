#include "DAPropertyItemWidget.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolTip>
#include <QEvent>

namespace DA
{
class DAPropertyItemWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAPropertyItemWidget)
public:
	PrivateData(DAPropertyItemWidget* p);

	int mPropertyId = -1;
	QString mPropertyName;
	QString mPropertyDescription;
	QWidget* mEditorWidget = nullptr;
	DAPropertyItemWidget::LayoutMode mLayoutMode = DAPropertyItemWidget::InlineLayout;
	int mNameLabelWidth = 100;
	bool mFrameVisible = false;

	QLabel* mLabelName = nullptr;
	QBoxLayout* mMainLayout = nullptr;

	void setupInlineLayout();
	void setupBelowLayout();
	void clearLayout();
	void updateTooltip();
};

//===================================================
// DAPropertyItemWidget::PrivateData
//===================================================

DAPropertyItemWidget::PrivateData::PrivateData(DAPropertyItemWidget* p) : q_ptr(p)
{
	if (p->objectName().isEmpty()) {
		p->setObjectName(QStringLiteral("DAPropertyItemWidget"));
	}
	// 默认Frame不可见
	p->setFrameShape(QFrame::NoFrame);

	// 创建属性名标签
	mLabelName = new QLabel(p);
	mLabelName->setWordWrap(true);
	mLabelName->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	mLabelName->setFixedWidth(mNameLabelWidth);
	mLabelName->setObjectName(QStringLiteral("labelPropertyName"));

	// 默认使用Inline布局
	setupInlineLayout();
}

void DAPropertyItemWidget::PrivateData::setupInlineLayout()
{
	clearLayout();

	mMainLayout = new QHBoxLayout(q_ptr);
	mMainLayout->setContentsMargins(2, 2, 2, 2);
	mMainLayout->setSpacing(4);
	mMainLayout->setObjectName(QStringLiteral("inlineLayout"));

	mLabelName->setFixedWidth(mNameLabelWidth);
	mLabelName->setWordWrap(true);
	mLabelName->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	mMainLayout->addWidget(mLabelName);

	if (mEditorWidget) {
		mMainLayout->addWidget(mEditorWidget, 1, Qt::AlignTop);
	}

	q_ptr->setLayout(mMainLayout);
}

void DAPropertyItemWidget::PrivateData::setupBelowLayout()
{
	clearLayout();

	mMainLayout = new QVBoxLayout(q_ptr);
	mMainLayout->setContentsMargins(2, 2, 2, 2);
	mMainLayout->setSpacing(2);
	mMainLayout->setObjectName(QStringLiteral("belowLayout"));

	// Below模式下标签不需要固定宽度，自适应宽度
	mLabelName->setFixedWidth(QWIDGETSIZE_MAX);
	mLabelName->setWordWrap(false);
	mLabelName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	mMainLayout->addWidget(mLabelName);

	if (mEditorWidget) {
		mMainLayout->addWidget(mEditorWidget, 1);
	}

	q_ptr->setLayout(mMainLayout);
}

void DAPropertyItemWidget::PrivateData::clearLayout()
{
	if (!mMainLayout) {
		return;
	}
	// 移除所有子项但保留mLabelName和mEditorWidget
	while (mMainLayout->count() > 0) {
		QLayoutItem* item = mMainLayout->takeAt(0);
		if (item->widget() != mLabelName && item->widget() != mEditorWidget) {
			delete item;
		} else {
			// 不删除mLabelName和mEditorWidget对应的layout item
			// 但需要从布局移除
			delete item;
		}
	}
	// 删除旧布局（QLayout删除时会清理）
	QLayout* oldLayout = q_ptr->layout();
	if (oldLayout) {
		// 先移除控件避免被layout删除
		if (mLabelName) {
			oldLayout->removeWidget(mLabelName);
		}
		if (mEditorWidget) {
			oldLayout->removeWidget(mEditorWidget);
		}
		delete oldLayout;
	}
	mMainLayout = nullptr;
}

void DAPropertyItemWidget::PrivateData::updateTooltip()
{
	if (mLabelName) {
		if (!mPropertyDescription.isEmpty()) {
			mLabelName->setToolTip(mPropertyDescription);
		} else {
			mLabelName->setToolTip(QString());
		}
	}
}

//===================================================
// DAPropertyItemWidget
//===================================================

DAPropertyItemWidget::DAPropertyItemWidget(QWidget* parent) : QFrame(parent), DA_PIMPL_CONSTRUCT
{
}

DAPropertyItemWidget::~DAPropertyItemWidget()
{
}

/**
 * @brief 获取属性ID
 * 
 * 返回当前属性项的唯一标识符。如果未通过setPropertyId()设置，
 * 默认值为-1表示未分配ID。
 * 
 * @return 属性ID
 * @see setPropertyId
 */
int DAPropertyItemWidget::propertyId() const
{
	DA_DC(d);
	return d->mPropertyId;
}

/**
 * @brief 设置属性ID
 * 
 * @param[in] id 属性ID，-1表示未分配
 * @see propertyId
 */
void DAPropertyItemWidget::setPropertyId(int id)
{
	DA_D(d);
	d->mPropertyId = id;
}

/**
 * @brief 获取属性名称
 * 
 * @return 属性名称文本
 * @see setPropertyName
 */
QString DAPropertyItemWidget::propertyName() const
{
	DA_DC(d);
	return d->mPropertyName;
}

/**
 * @brief 设置属性名称
 * 
 * 设置属性名标签的文本内容。在Inline模式下，文本会在固定宽度内换行显示；
 * 在Below模式下，文本自适应宽度显示。
 * 
 * @param[in] name 属性名称
 * @see propertyName
 */
void DAPropertyItemWidget::setPropertyName(const QString& name)
{
	DA_D(d);
	d->mPropertyName = name;
	if (d->mLabelName) {
		d->mLabelName->setText(name);
	}
}

/**
 * @brief 获取属性描述
 * 
 * 属性描述作为属性名标签的tooltip显示。
 * 
 * @return 属性描述文本
 * @see setPropertyDescription
 */
QString DAPropertyItemWidget::propertyDescription() const
{
	DA_DC(d);
	return d->mPropertyDescription;
}

/**
 * @brief 设置属性描述
 * 
 * 设置属性名标签的tooltip文本。鼠标悬停在属性名标签上时显示此描述。
 * Inline和Below模式统一使用tooltip显示描述信息。
 * 
 * @param[in] desc 属性描述文本
 * @see propertyDescription
 */
void DAPropertyItemWidget::setPropertyDescription(const QString& desc)
{
	DA_D(d);
	d->mPropertyDescription = desc;
	d->updateTooltip();
}

/**
 * @brief 获取编辑Widget
 * 
 * @return 编辑Widget指针，未设置则返回nullptr
 * @see setEditorWidget
 */
QWidget* DAPropertyItemWidget::editorWidget() const
{
	DA_DC(d);
	return d->mEditorWidget;
}

/**
 * @brief 设置编辑Widget
 * 
 * 设置属性项的编辑Widget。此方法会接管widget的父子关系，
 * 将其设为DAPropertyItemWidget的子控件，并添加到当前布局中。
 * 
 * 如果之前已设置编辑Widget，旧Widget会被移除但不会删除，
 * 调用者需自行管理旧Widget的生命周期。
 * 
 * @param[in] widget 编辑Widget指针
 * @see editorWidget
 */
void DAPropertyItemWidget::setEditorWidget(QWidget* widget)
{
	DA_D(d);
	// 移除旧编辑Widget
	if (d->mEditorWidget) {
		d->mMainLayout->removeWidget(d->mEditorWidget);
		d->mEditorWidget->setParent(nullptr);
	}
	d->mEditorWidget = widget;
	if (widget) {
		widget->setParent(this);
		widget->setObjectName(QStringLiteral("editorWidget"));
		if (d->mMainLayout) {
			if (d->mLayoutMode == InlineLayout) {
				d->mMainLayout->addWidget(widget, 1, Qt::AlignTop);
			} else {
				d->mMainLayout->addWidget(widget, 1);
			}
		}
	}
}

/**
 * @brief 获取布局模式
 * 
 * @return 当前布局模式
 * @see setLayoutMode
 */
DAPropertyItemWidget::LayoutMode DAPropertyItemWidget::layoutMode() const
{
	DA_DC(d);
	return d->mLayoutMode;
}

/**
 * @brief 设置布局模式
 * 
 * 支持动态切换Inline和Below布局模式。切换时会重建布局结构，
 * 保留属性名标签和编辑Widget。
 * 
 * @param[in] mode 布局模式
 * @see layoutMode
 */
void DAPropertyItemWidget::setLayoutMode(LayoutMode mode)
{
	DA_D(d);
	if (d->mLayoutMode == mode) {
		return;
	}
	d->mLayoutMode = mode;
	if (mode == InlineLayout) {
		d->setupInlineLayout();
	} else {
		d->setupBelowLayout();
	}
}

/**
 * @brief 获取Frame边界可见性
 * 
 * @return true表示Frame可见，false表示不可见（默认）
 * @see setFrameVisible
 */
bool DAPropertyItemWidget::isFrameVisible() const
{
	DA_DC(d);
	return d->mFrameVisible;
}

/**
 * @brief 设置Frame边界可见性
 * 
 * 默认Frame不可见（NoFrame）。设置为可见时使用StyledPanel样式。
 * 
 * @param[in] visible 是否可见
 * @see isFrameVisible
 */
void DAPropertyItemWidget::setFrameVisible(bool visible)
{
	DA_D(d);
	d->mFrameVisible = visible;
	if (visible) {
		setFrameShape(QFrame::StyledPanel);
	} else {
		setFrameShape(QFrame::NoFrame);
	}
}

/**
 * @brief 设置属性名标签宽度
 * 
 * 仅在Inline模式下有效。设置属性名标签的固定宽度，
 * 超出此宽度的文本会自动换行显示。
 * 
 * @param[in] width 标签宽度（像素）
 */
void DAPropertyItemWidget::setNameLabelWidth(int width)
{
	DA_D(d);
	d->mNameLabelWidth = width;
	if (d->mLayoutMode == InlineLayout && d->mLabelName) {
		d->mLabelName->setFixedWidth(width);
	}
}

void DAPropertyItemWidget::updateLayout()
{
	DA_D(d);
	if (d->mLayoutMode == InlineLayout) {
		d->setupInlineLayout();
	} else {
		d->setupBelowLayout();
	}
}

}  // namespace DA