#include "DANodeItemSettingWidget.h"
#include <QDebug>
#include <QSignalBlocker>
#include <QPointer>
#include <QButtonGroup>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include "DAPropertyPanelContainerWidget.h"
#include "DAPyWorkFlowGraphicsScene.h"
#include "DAGraphicsResizeableItem.h"
#include "DACommandsForGraphics.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkPoint.h"
#include "DAGraphicsCommandsFactory.h"
namespace DA
{
class DANodeItemSettingWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DANodeItemSettingWidget)
public:
	PrivateData(DANodeItemSettingWidget* p);
	QPointer< DAGraphicsResizeableItem > mItem { nullptr };
	QPointer< DAPyWorkFlowGraphicsScene > mScene { nullptr };
	DAPropertyPanelContainerWidget* mPanel { nullptr };
	QButtonGroup* mButtonGroupInputLocation { nullptr };
	QButtonGroup* mButtonGroupOutputLocation { nullptr };
	QToolButton* toolButtonInLeft { nullptr };
	QToolButton* toolButtonInTop { nullptr };
	QToolButton* toolButtonInRight { nullptr };
	QToolButton* toolButtonInBottom { nullptr };
	QToolButton* toolButtonOutLeft { nullptr };
	QToolButton* toolButtonOutTop { nullptr };
	QToolButton* toolButtonOutRight { nullptr };
	QToolButton* toolButtonOutBottom { nullptr };
};

DANodeItemSettingWidget::PrivateData::PrivateData(DANodeItemSettingWidget* p) : q_ptr(p)
{
}

/////////////////////////////////////////////////
/// DANodeItemSettingWidget
/////////////////////////////////////////////////

/**
 * @brief 构造函数
 *
 * 创建属性面板并构建所有属性项，连接信号槽。
 *
 * @param[in] parent 父控件
 */
DANodeItemSettingWidget::DANodeItemSettingWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
	buildPanel();
}

DANodeItemSettingWidget::~DANodeItemSettingWidget()
{
}

/**
 * @brief 构建属性面板
 *
 * 使用DAPropertyPanelContainerWidget创建属性面板，按分组添加所有属性项：
 * - 尺寸组：宽度、高度、锁定纵横比、旋转
 * - 位置组：X坐标、Y坐标
 * - 属性组：可移动、可缩放
 * - 提示文本
 * - 连接点位置组（自定义widget）
 */
void DANodeItemSettingWidget::buildPanel()
{
	DA_D(d);
	d->mPanel = new DAPropertyPanelContainerWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(d->mPanel);

	// 尺寸组
	d->mPanel->addCollapsibleGroup(QString::fromUtf8("尺寸"));
	d->mPanel->addDoubleProperty(PropWidth, QString::fromUtf8("宽度"), 0, 0, 99999, 2);
	d->mPanel->addDoubleProperty(PropHeight, QString::fromUtf8("高度"), 0, 0, 99999, 2);
	d->mPanel->addBoolProperty(PropLockAspectRatio, QString::fromUtf8("锁定纵横比"), false);
	d->mPanel->addSeparator();
	d->mPanel->addDoubleProperty(PropRotation, QString::fromUtf8("旋转"), 0, 0, 360, 1);
	d->mPanel->endGroup();

	// 位置组
	d->mPanel->addCollapsibleGroup(QString::fromUtf8("位置"));
	d->mPanel->addDoubleProperty(PropX, QString::fromUtf8("X"), 0, -999999, 9999999, 2);
	d->mPanel->addDoubleProperty(PropY, QString::fromUtf8("Y"), 0, -9999999, 9999999, 2);
	d->mPanel->endGroup();

	// 属性组
	d->mPanel->addCollapsibleGroup(QString::fromUtf8("属性"));
	d->mPanel->addBoolProperty(PropMovable, QString::fromUtf8("可移动"), false);
	d->mPanel->addBoolProperty(PropResizable, QString::fromUtf8("可缩放"), false);
	d->mPanel->endGroup();

	// 提示文本
	d->mPanel->addStringProperty(PropTooltip, QString::fromUtf8("提示"), QString());

	// 连接点位置组
	d->mPanel->addCollapsibleGroup(QString::fromUtf8("连接点位置"));
	QWidget* linkPointWidget = createLinkPointWidget();
	d->mPanel->addProperty(QString::fromUtf8("方向"), linkPointWidget);
	d->mPanel->endGroup();

	// 连接属性值变化信号
	connect(d->mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged, this,
			&DANodeItemSettingWidget::onPropertyValueChanged);
}

/**
 * @brief 创建连接点方向选择控件
 *
 * 创建包含输入/输出方向按钮的自定义widget，使用图标按钮表示四个方向。
 * 每组按钮使用QButtonGroup管理互斥选择。
 *
 * @return 包含方向按钮的widget
 */
QWidget* DANodeItemSettingWidget::createLinkPointWidget()
{
	DA_D(d);

	QWidget* linkPointWidget = new QWidget();
	QVBoxLayout* linkPointLayout = new QVBoxLayout(linkPointWidget);
	linkPointLayout->setContentsMargins(0, 0, 0, 0);
	linkPointLayout->setSpacing(2);

	// 输入方向
	QLabel* inputLabel = new QLabel(QString::fromUtf8("输入方向"), linkPointWidget);
	linkPointLayout->addWidget(inputLabel);

	QHBoxLayout* inputLayout = new QHBoxLayout();
	inputLayout->setSpacing(2);

	d->toolButtonInLeft = new QToolButton();
	d->toolButtonInLeft->setIcon(QIcon(QString::fromUtf8(":/DAGui/icon/leftSideIn.svg")));
	d->toolButtonInLeft->setCheckable(true);
	d->toolButtonInLeft->setAutoRaise(true);

	d->toolButtonInTop = new QToolButton();
	d->toolButtonInTop->setIcon(QIcon(QString::fromUtf8(":/DAGui/icon/topSideIn.svg")));
	d->toolButtonInTop->setCheckable(true);
	d->toolButtonInTop->setAutoRaise(true);

	d->toolButtonInRight = new QToolButton();
	d->toolButtonInRight->setIcon(QIcon(QString::fromUtf8(":/DAGui/icon/rightSideIn.svg")));
	d->toolButtonInRight->setCheckable(true);
	d->toolButtonInRight->setAutoRaise(true);

	d->toolButtonInBottom = new QToolButton();
	d->toolButtonInBottom->setIcon(QIcon(QString::fromUtf8(":/DAGui/icon/bottomSideIn.svg")));
	d->toolButtonInBottom->setCheckable(true);
	d->toolButtonInBottom->setAutoRaise(true);

	inputLayout->addWidget(d->toolButtonInLeft);
	inputLayout->addWidget(d->toolButtonInTop);
	inputLayout->addWidget(d->toolButtonInRight);
	inputLayout->addWidget(d->toolButtonInBottom);
	inputLayout->addStretch();
	linkPointLayout->addLayout(inputLayout);

	// 输出方向
	QLabel* outputLabel = new QLabel(QString::fromUtf8("输出方向"), linkPointWidget);
	linkPointLayout->addWidget(outputLabel);

	QHBoxLayout* outputLayout = new QHBoxLayout();
	outputLayout->setSpacing(2);

	d->toolButtonOutLeft = new QToolButton();
	d->toolButtonOutLeft->setIcon(QIcon(QString::fromUtf8(":/DAGui/icon/leftSideOut.svg")));
	d->toolButtonOutLeft->setCheckable(true);
	d->toolButtonOutLeft->setAutoRaise(true);

	d->toolButtonOutTop = new QToolButton();
	d->toolButtonOutTop->setIcon(QIcon(QString::fromUtf8(":/DAGui/icon/topSideOut.svg")));
	d->toolButtonOutTop->setCheckable(true);
	d->toolButtonOutTop->setAutoRaise(true);

	d->toolButtonOutRight = new QToolButton();
	d->toolButtonOutRight->setIcon(QIcon(QString::fromUtf8(":/DAGui/icon/rightSideOut.svg")));
	d->toolButtonOutRight->setCheckable(true);
	d->toolButtonOutRight->setAutoRaise(true);

	d->toolButtonOutBottom = new QToolButton();
	d->toolButtonOutBottom->setIcon(QIcon(QString::fromUtf8(":/DAGui/icon/bottomSideOut.svg")));
	d->toolButtonOutBottom->setCheckable(true);
	d->toolButtonOutBottom->setAutoRaise(true);

	outputLayout->addWidget(d->toolButtonOutLeft);
	outputLayout->addWidget(d->toolButtonOutTop);
	outputLayout->addWidget(d->toolButtonOutRight);
	outputLayout->addWidget(d->toolButtonOutBottom);
	outputLayout->addStretch();
	linkPointLayout->addLayout(outputLayout);

	// 输入方向按钮组
	d->mButtonGroupInputLocation = new QButtonGroup(this);
	d->mButtonGroupInputLocation->setExclusive(true);
	d->mButtonGroupInputLocation->addButton(d->toolButtonInLeft, 0);
	d->mButtonGroupInputLocation->addButton(d->toolButtonInTop, 1);
	d->mButtonGroupInputLocation->addButton(d->toolButtonInRight, 2);
	d->mButtonGroupInputLocation->addButton(d->toolButtonInBottom, 3);

	// 输出方向按钮组
	d->mButtonGroupOutputLocation = new QButtonGroup(this);
	d->mButtonGroupOutputLocation->setExclusive(true);
	d->mButtonGroupOutputLocation->addButton(d->toolButtonOutLeft, 4);
	d->mButtonGroupOutputLocation->addButton(d->toolButtonOutTop, 5);
	d->mButtonGroupOutputLocation->addButton(d->toolButtonOutRight, 6);
	d->mButtonGroupOutputLocation->addButton(d->toolButtonOutBottom, 7);

	// Qt5/Qt6兼容的按钮组信号连接
#if QT_VERSION_MAJOR >= 6
	connect(d->mButtonGroupInputLocation, &QButtonGroup::idClicked, this, &DANodeItemSettingWidget::onButtonGroupClicked);
	connect(d->mButtonGroupOutputLocation, &QButtonGroup::idClicked, this, &DANodeItemSettingWidget::onButtonGroupClicked);
#else
	connect(d->mButtonGroupInputLocation, QOverload< int >::of(&QButtonGroup::buttonPressed), this,
			&DANodeItemSettingWidget::onButtonGroupClicked);
	connect(d->mButtonGroupOutputLocation, QOverload< int >::of(&QButtonGroup::buttonPressed), this,
			&DANodeItemSettingWidget::onButtonGroupClicked);
#endif

	return linkPointWidget;
}

/**
 * @brief 设置需要配置的item
 *
 * @param[in] item 需要配置的DAGraphicsResizeableItem
 */
void DANodeItemSettingWidget::setItem(DAGraphicsResizeableItem* item)
{
	d_ptr->mItem = item;
	updateData();
}

/**
 * @brief 获取维护的item
 * @return 当前设置的item指针
 */
DAGraphicsResizeableItem* DANodeItemSettingWidget::getItem() const
{
	return d_ptr->mItem.data();
}

/**
 * @brief 设置了scene能实现redo/undo
 *
 * 如果设置nullptr，将不使用redo/undo
 * @param[in] sc 工作流场景指针
 */
void DANodeItemSettingWidget::setScene(DAPyWorkFlowGraphicsScene* sc)
{
	if (d_ptr->mScene == sc) {
		return;
	}
	if (d_ptr->mScene) {
		disconnect(d_ptr->mScene.data(), &DAPyWorkFlowScene::pyNodeItemsRemoved, this,
				   &DANodeItemSettingWidget::onNodeItemsRemoved);
	}
	d_ptr->mScene = sc;
	if (d_ptr->mScene) {
		connect(d_ptr->mScene.data(), &DAPyWorkFlowScene::pyNodeItemsRemoved, this,
				&DANodeItemSettingWidget::onNodeItemsRemoved);
	}
}

/**
 * @brief 更新所有属性面板数据
 *
 * 当item为nullptr时重置所有值，否则依次更新位置、尺寸、状态、旋转和连接点位置。
 */
void DANodeItemSettingWidget::updateData()
{
	if (nullptr == d_ptr->mItem) {
		resetValue();
		return;
	}
	updatePosition();
	updateBodySize();
	updateItemState();
	updateRotation();
	updateLinkPointLocation();
}

/**
 * @brief 更新位置属性值
 *
 * 使用QSignalBlocker防止程序更新触发信号，读取item的x/y坐标并设置到面板。
 */
void DANodeItemSettingWidget::updatePosition()
{
	QSignalBlocker blocker(d_ptr->mPanel);
	d_ptr->mPanel->setDoubleValue(PropX, d_ptr->mItem->x());
	d_ptr->mPanel->setDoubleValue(PropY, d_ptr->mItem->y());
}

/**
 * @brief 更新旋转属性值
 *
 * 使用QSignalBlocker防止程序更新触发信号，读取item的旋转角度并设置到面板。
 */
void DANodeItemSettingWidget::updateRotation()
{
	QSignalBlocker blocker(d_ptr->mPanel);
	d_ptr->mPanel->setDoubleValue(PropRotation, d_ptr->mItem->rotation());
}

/**
 * @brief 更新body尺寸属性值
 *
 * 使用QSignalBlocker防止程序更新触发信号，读取item的body尺寸、最小和最大尺寸，
 * 并更新面板中的宽度和高度值及范围。
 */
void DANodeItemSettingWidget::updateBodySize()
{
	QSignalBlocker blocker(d_ptr->mPanel);
	QSizeF originSize = d_ptr->mItem->getBodySize();
	QSizeF minSize    = d_ptr->mItem->getBodyMinimumSize();
	QSizeF maxSize    = d_ptr->mItem->getBodyMaximumSize();
	// 注意：addDoubleProperty在构造时已设置初始范围，这里需要通过底层spinbox更新min/max
	DAPropertyItemWidget* widthItem = d_ptr->mPanel->getPropertyItem(PropWidth);
	DAPropertyItemWidget* heightItem = d_ptr->mPanel->getPropertyItem(PropHeight);
	if (widthItem) {
		QDoubleSpinBox* widthSpin = widthItem->findChild< QDoubleSpinBox* >();
		if (widthSpin) {
			widthSpin->setMinimum(minSize.width());
			widthSpin->setMaximum(maxSize.width());
		}
	}
	if (heightItem) {
		QDoubleSpinBox* heightSpin = heightItem->findChild< QDoubleSpinBox* >();
		if (heightSpin) {
			heightSpin->setMinimum(minSize.height());
			heightSpin->setMaximum(maxSize.height());
		}
	}
	d_ptr->mPanel->setDoubleValue(PropWidth, originSize.width());
	d_ptr->mPanel->setDoubleValue(PropHeight, originSize.height());
}

/**
 * @brief 更新item状态属性
 *
 * 使用QSignalBlocker防止程序更新触发信号，读取item的可移动/可缩放状态和提示文本，
 * 并根据场景网格尺寸设置位置spinbox的步长。同时根据可移动/可缩放状态启用/禁用相关属性。
 */
void DANodeItemSettingWidget::updateItemState()
{
	QSignalBlocker blocker(d_ptr->mPanel);
	// 网格的尺寸作为x,y的步长
	if (d_ptr->mScene) {
		QSize gs = d_ptr->mScene->getGridSize();
		DAPropertyItemWidget* xItem = d_ptr->mPanel->getPropertyItem(PropX);
		DAPropertyItemWidget* yItem = d_ptr->mPanel->getPropertyItem(PropY);
		if (xItem) {
			QDoubleSpinBox* xSpin = xItem->findChild< QDoubleSpinBox* >();
			if (xSpin) {
				xSpin->setSingleStep(gs.width());
			}
		}
		if (yItem) {
			QDoubleSpinBox* ySpin = yItem->findChild< QDoubleSpinBox* >();
			if (ySpin) {
				ySpin->setSingleStep(gs.height());
			}
		}
	}
	d_ptr->mPanel->setBoolValue(PropMovable, d_ptr->mItem->isMovable());
	d_ptr->mPanel->setBoolValue(PropResizable, d_ptr->mItem->isResizable());
	d_ptr->mPanel->setPropertyEnabled(PropWidth, d_ptr->mItem->isResizable());
	d_ptr->mPanel->setPropertyEnabled(PropHeight, d_ptr->mItem->isResizable());
	d_ptr->mPanel->setPropertyEnabled(PropX, d_ptr->mItem->isMovable());
	d_ptr->mPanel->setPropertyEnabled(PropY, d_ptr->mItem->isMovable());
	d_ptr->mPanel->setStringValue(PropTooltip, d_ptr->mItem->toolTip());
}

/**
 * @brief 更新连接点的位置
 *
 * 读取DAPyNodeGraphicsItem的输入/输出连接点方向，设置对应的按钮选中状态。
 * 如果item不是DAPyNodeGraphicsItem，则跳过。
 */
void DANodeItemSettingWidget::updateLinkPointLocation()
{
	if (nullptr == d_ptr->mItem) {
		return;
	}
	DAPyNodeGraphicsItem* nodeItem = qobject_cast< DAPyNodeGraphicsItem* >(d_ptr->mItem);
	if (nullptr == nodeItem) {
		// 说明不是node link
		return;
	}
	QSignalBlocker b1(d_ptr->mButtonGroupInputLocation), b2(d_ptr->mButtonGroupOutputLocation);
	Q_UNUSED(b1);
	Q_UNUSED(b2);
	// TODO: DAPyNodeGraphicsItem needs getLinkPointDirection/getLinkPointLocation API
	// For now, check the direction of the first input/output link point
	QList< DAPyLinkPoint > inputPoints = nodeItem->getInputLinkPoints();
	AspectDirection inputDir = (inputPoints.isEmpty()) ? AspectDirection::West : inputPoints.first().direction;
	switch (inputDir) {
	case AspectDirection::West:
		d_ptr->toolButtonInLeft->setChecked(true);
		break;
	case AspectDirection::North:
		d_ptr->toolButtonInTop->setChecked(true);
		break;
	case AspectDirection::East:
		d_ptr->toolButtonInRight->setChecked(true);
		break;
	case AspectDirection::South:
		d_ptr->toolButtonInBottom->setChecked(true);
		break;
	default:
		break;
	}
	QList< DAPyLinkPoint > outputPoints = nodeItem->getOutputLinkPoints();
	AspectDirection outputDir = (outputPoints.isEmpty()) ? AspectDirection::East : outputPoints.first().direction;
	switch (outputDir) {
	case AspectDirection::West:
		d_ptr->toolButtonOutLeft->setChecked(true);
		break;
	case AspectDirection::North:
		d_ptr->toolButtonOutTop->setChecked(true);
		break;
	case AspectDirection::East:
		d_ptr->toolButtonOutRight->setChecked(true);
		break;
	case AspectDirection::South:
		d_ptr->toolButtonOutBottom->setChecked(true);
		break;
	default:
		break;
	}
}

/**
 * @brief 属性值变化处理
 *
 * 根据propertyId分发到对应的处理逻辑，包括位置、尺寸、旋转、状态等属性的变更。
 * 对于尺寸变更，支持锁定纵横比模式下的联动更新。
 * 对于位置变更，支持网格对齐后的位置回读。
 * 如果设置了scene，将变更推入undo/redo命令栈。
 *
 * @param[in] propertyId 发生变化的属性ID
 */
void DANodeItemSettingWidget::onPropertyValueChanged(int propertyId)
{
	DA_D(d);
	if (nullptr == d->mItem) {
		return;
	}
	switch (propertyId) {
	case PropWidth: {
		double v             = d->mPanel->getDoubleValue(PropWidth);
		QSizeF originSize    = d->mItem->getBodySize();
		QSizeF willSetSize;
		if (d->mPanel->getBoolValue(PropLockAspectRatio)) {
			willSetSize = originSize.scaled(v, originSize.height(), Qt::KeepAspectRatio);
		} else {
			willSetSize = QSizeF(v, originSize.height());
		}
		if (d->mScene) {
			if (originSize != willSetSize) {
				auto cmd = d->mScene->commandsFactory()->createItemResized(d->mItem, originSize, willSetSize);
				d->mScene->push(cmd);
			}
		} else {
			d->mItem->setBodySize(willSetSize);
		}
		QSizeF settedSize = d->mItem->getBodySize();
		QSignalBlocker blocker(d->mPanel);
		d->mPanel->setDoubleValue(PropWidth, settedSize.width());
		d->mPanel->setDoubleValue(PropHeight, settedSize.height());
		break;
	}
	case PropHeight: {
		double v          = d->mPanel->getDoubleValue(PropHeight);
		QSizeF originSize = d->mItem->getBodySize();
		QSizeF willSetSize;
		if (d->mPanel->getBoolValue(PropLockAspectRatio)) {
			willSetSize = originSize.scaled(originSize.width(), v, Qt::KeepAspectRatio);
		} else {
			willSetSize = QSizeF(originSize.width(), v);
		}
		if (d->mScene) {
			if (d->mItem->getBodySize().height() == originSize.height()) {
				// 说明没设置成功,跳过
				return;
			}
			auto cmd = d->mScene->commandsFactory()->createItemResized(d->mItem, originSize, willSetSize);
			d->mScene->push(cmd);
		} else {
			d->mItem->setBodySize(willSetSize);
		}
		// 设置成功了，同步更新面板值
		QSizeF settedSize = d->mItem->getBodySize();
		QSignalBlocker blocker(d->mPanel);
		d->mPanel->setDoubleValue(PropWidth, settedSize.width());
		d->mPanel->setDoubleValue(PropHeight, settedSize.height());
		break;
	}
	case PropRotation: {
		double v          = d->mPanel->getDoubleValue(PropRotation);
		qreal oldRotation = d->mItem->rotation();
		if (qFuzzyCompare(oldRotation, v)) {
			return;
		}
		if (d->mScene) {
			if (!qFuzzyCompare(d->mItem->rotation(), v)) {
				// 说明没设置成功,回读实际值
				QSignalBlocker blocker(d->mPanel);
				d->mPanel->setDoubleValue(PropRotation, d->mItem->rotation());
				return;
			}
			auto cmd = d->mScene->commandsFactory()->createItemRotation(d->mItem, oldRotation, v);
			d->mScene->push(cmd);
		} else {
			d->mItem->setRotation(v);
		}
		break;
	}
	case PropX: {
		double v      = d->mPanel->getDoubleValue(PropX);
		QPointF oldpos = d->mItem->pos();
		d->mItem->setScenePos(v, oldpos.y());
		QPointF newPos = d->mItem->pos();
		// 在存在网格的情况下，可能设置的位置不是显示的位置
		QSignalBlocker blocker(d->mPanel);
		d->mPanel->setDoubleValue(PropX, newPos.x());
		d->mPanel->setDoubleValue(PropY, newPos.y());
		if (d->mScene) {
			auto cmd = d->mScene->commandsFactory()->createItemMoved(d->mItem, oldpos, newPos, true);
			d->mScene->push(cmd);
		}
		break;
	}
	case PropY: {
		double v      = d->mPanel->getDoubleValue(PropY);
		QPointF oldpos = d->mItem->pos();
		d->mItem->setScenePos(oldpos.x(), v);
		QPointF newPos = d->mItem->pos();
		QSignalBlocker blocker(d->mPanel);
		d->mPanel->setDoubleValue(PropX, newPos.x());
		d->mPanel->setDoubleValue(PropY, newPos.y());
		if (d->mScene) {
			auto cmd = d->mScene->commandsFactory()->createItemMoved(d->mItem, oldpos, newPos, true);
			d->mScene->push(cmd);
		}
		break;
	}
	case PropMovable: {
		bool movable = d->mPanel->getBoolValue(PropMovable);
		d->mItem->setFlag(QGraphicsItem::ItemIsMovable, movable);
		d->mPanel->setPropertyEnabled(PropX, movable);
		d->mPanel->setPropertyEnabled(PropY, movable);
		break;
	}
	case PropResizable: {
		bool resizable = d->mPanel->getBoolValue(PropResizable);
		d->mItem->setEnableResize(resizable);
		d->mItem->update();
		d->mPanel->setPropertyEnabled(PropWidth, resizable);
		d->mPanel->setPropertyEnabled(PropHeight, resizable);
		break;
	}
	case PropTooltip: {
		QString t = d->mPanel->getStringValue(PropTooltip);
		d->mItem->setToolTip(t);
		break;
	}
	case PropLockAspectRatio:
		// 锁定纵横比仅影响宽度/高度联动，无需单独处理
		break;
	default:
		break;
	}
}

/**
 * @brief 节点项被移除的处理
 *
 * 当监听的item被从场景中移除时，清空item引用并重置面板。
 *
 * @param[in] items 被移除的节点项列表
 */
void DANodeItemSettingWidget::onNodeItemsRemoved(const QList< DAPyNodeGraphicsItem* >& items)
{
	if (d_ptr->mItem) {
		for (const DAPyNodeGraphicsItem* i : std::as_const(items)) {
			if (i == d_ptr->mItem.data()) {
				d_ptr->mItem = nullptr;
				updateData();
			}
		}
	}
}

/**
 * @brief 连接点方向按钮点击处理
 *
 * 根据按钮ID确定输入/输出方向，更新节点的连接点方向并刷新显示。
 * ID 0-3对应输入方向（西/北/东/南），ID 4-7对应输出方向。
 *
 * @param[in] id 按钮组ID
 */
void DANodeItemSettingWidget::onButtonGroupClicked(int id)
{
	if (nullptr == d_ptr->mItem) {
		return;
	}
	DAPyNodeGraphicsItem* nodeItem = qobject_cast< DAPyNodeGraphicsItem* >(d_ptr->mItem);
	if (nullptr == nodeItem) {
		return;
	}
	switch (id) {
	case 0:
		// TODO: DAPyNodeGraphicsItem needs setLinkPointDirection API
		// nodeItem->setLinkPointDirection(DAPyLinkPoint::Way::Input, AspectDirection::West);
		break;
	case 1:
		// nodeItem->setLinkPointDirection(DAPyLinkPoint::Way::Input, AspectDirection::North);
		break;
	case 2:
		// nodeItem->setLinkPointDirection(DAPyLinkPoint::Way::Input, AspectDirection::East);
		break;
	case 3:
		// nodeItem->setLinkPointDirection(DAPyLinkPoint::Way::Input, AspectDirection::South);
		break;
	case 4:
		// nodeItem->setLinkPointDirection(DAPyLinkPoint::Way::Output, AspectDirection::West);
		break;
	case 5:
		// nodeItem->setLinkPointDirection(DAPyLinkPoint::Way::Output, AspectDirection::North);
		break;
	case 6:
		// nodeItem->setLinkPointDirection(DAPyLinkPoint::Way::Output, AspectDirection::East);
		break;
	case 7:
		// nodeItem->setLinkPointDirection(DAPyLinkPoint::Way::Output, AspectDirection::South);
		break;
	default:
		break;
	}
	nodeItem->updateLinkPoints();
	nodeItem->update();
}

/**
 * @brief 重置所有属性面板值
 *
 * 将所有属性值恢复为默认值（0、false、空字符串）。
 */
void DANodeItemSettingWidget::resetValue()
{
	QSignalBlocker blocker(d_ptr->mPanel);
	d_ptr->mPanel->setDoubleValue(PropWidth, 0);
	d_ptr->mPanel->setDoubleValue(PropHeight, 0);
	d_ptr->mPanel->setDoubleValue(PropRotation, 0);
	d_ptr->mPanel->setDoubleValue(PropX, 0);
	d_ptr->mPanel->setDoubleValue(PropY, 0);
	d_ptr->mPanel->setBoolValue(PropMovable, false);
	d_ptr->mPanel->setBoolValue(PropResizable, false);
	d_ptr->mPanel->setBoolValue(PropLockAspectRatio, false);
	d_ptr->mPanel->setStringValue(PropTooltip, QString());
}

}  // end of DA