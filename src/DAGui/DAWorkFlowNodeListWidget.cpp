#include "DAWorkFlowNodeListWidget.h"
#include <QActionGroup>
#include <QDrag>
#include "ui_DAWorkFlowNodeListWidget.h"
#include "DANodeListWidget.h"
#include "DAToolBox.h"
#include "DANodeListWidget.h"
#include "DANodeTreeWidget.h"
#include "DANodeMimeData.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAWorkFlowNodeListWidget
//===================================================
DAWorkFlowNodeListWidget::DAWorkFlowNodeListWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAWorkFlowNodeListWidget), _menu(nullptr)
{
	ui->setupUi(this);
	_actionViewNodeListByToolBox = new QAction(this);
	_actionViewNodeListByToolBox->setObjectName("actionViewDataListByTable");
	_actionViewNodeListByToolBox->setCheckable(true);
	_actionViewNodeListByToolBox->setIcon(QIcon(":/DAGui/icon/showDataInList.svg"));
	_actionViewNodeListByTree = new QAction(this);
	_actionViewNodeListByTree->setObjectName("actionViewDataListByTree");
	_actionViewNodeListByTree->setCheckable(true);
	_actionViewNodeListByTree->setIcon(QIcon(":/DAGui/icon/showDataInTree.svg"));
	_actionGroup = new QActionGroup(this);
	_actionGroup->addAction(_actionViewNodeListByToolBox);
	_actionGroup->addAction(_actionViewNodeListByTree);
	_actionGroup->setExclusive(true);
	ui->toolButtonList->setDefaultAction(_actionViewNodeListByToolBox);
	ui->toolButtonTree->setDefaultAction(_actionViewNodeListByTree);
	setDisplayMode(DisplayInToolBox);
	connect(this,
			&DAWorkFlowNodeListWidget::customContextMenuRequested,
			this,
			&DAWorkFlowNodeListWidget::onCustomContextMenuRequested);
	connect(_actionGroup, &QActionGroup::triggered, this, &DAWorkFlowNodeListWidget::onActionGroupTriggered);
}

DAWorkFlowNodeListWidget::~DAWorkFlowNodeListWidget()
{
    delete ui;
}

/**
 * @brief 添加节点，并自动分组
 * @param datas
 */
void DAWorkFlowNodeListWidget::addItems(const QList< DANodeMetaData >& datas)
{
	ui->workFlowToolBox->addItems(datas);
	ui->workflowTreeWidget->addItems(datas);
}

/**
 * @brief 设置显示模式
 * @param m
 */
void DAWorkFlowNodeListWidget::setDisplayMode(DAWorkFlowNodeListWidget::DisplayMode m)
{
	switch (m) {
	case DisplayInToolBox:
		_actionViewNodeListByToolBox->setChecked(true);
		ui->stackedWidget->setCurrentWidget(ui->workFlowToolBox);
		break;
	default:
		_actionViewNodeListByTree->setChecked(true);
		ui->stackedWidget->setCurrentWidget(ui->workflowTreeWidget);
		break;
	}
}

/**
 * @brief 获取当前的显示模式
 * @return
 */
DAWorkFlowNodeListWidget::DisplayMode DAWorkFlowNodeListWidget::getDisplayMode() const
{
	if (ui->stackedWidget->currentWidget() == ui->workFlowToolBox) {
		return DisplayInToolBox;
	}
	return DisplayInTree;
}

DAToolBox* DAWorkFlowNodeListWidget::getToolBox() const
{
	return ui->workFlowToolBox;
}

DANodeTreeWidget* DAWorkFlowNodeListWidget::getTreeWidget() const
{
    return ui->workflowTreeWidget;
}

/**
 * @brief 创建拖曳
 * @param parent
 * @param md
 * @return
 */
QDrag* DAWorkFlowNodeListWidget::createDrag(QObject* parent, const DANodeMetaData& md)
{
	DANodeMimeData* mimedata = new DANodeMimeData(md);
	QDrag* drag              = new QDrag(parent);
	drag->setMimeData(mimedata);
	drag->setPixmap(md.getIcon().pixmap(36, 36));
	drag->setHotSpot(QPoint(18, 18));
	return drag;
}

/**
 * @brief 构建菜单
 */
void DAWorkFlowNodeListWidget::buildMenu()
{
	_menu                 = new QMenu(this);
	_actionAddFavorite    = new QAction(QIcon(":/DAGui/icon/favorite.svg"), tr("Favorite"), this);
	_actionRemoveFavorite = new QAction(QIcon(":/DAGui/icon/removeFavorite.svg"), tr("Remove Favorite"), this);
	_menu->addAction(_actionAddFavorite);
	_menu->addAction(_actionRemoveFavorite);
	connect(_actionAddFavorite, &QAction::triggered, this, &DAWorkFlowNodeListWidget::onActionAddFavoriteTriggered);
	connect(_actionRemoveFavorite, &QAction::triggered, this, &DAWorkFlowNodeListWidget::onActionRemoveFavoriteTriggered);
}

/**
 * @brief 鼠标右键
 * @param pos
 */
void DAWorkFlowNodeListWidget::onCustomContextMenuRequested(const QPoint& pos)
{
	if (!_menu) {
		buildMenu();
	}
	_lastCustoRequestedPoint = pos;
	if (DisplayInToolBox == getDisplayMode()) {
		DAToolBox* tb = qobject_cast< DAToolBox* >(ui->stackedWidget->currentWidget());
		if (!tb || !tb->underMouse()) {
			return;
		}
		DANodeListWidget* nl = qobject_cast< DANodeListWidget* >(tb->currentWidget());
		if (!nl) {
			return;
		}
		if (nl == tb->getFavoriteList()) {
			// 如果这个页面就是收藏页面
			_actionAddFavorite->setEnabled(false);
			_actionRemoveFavorite->setEnabled(true);
		} else {
			_actionAddFavorite->setEnabled(true);
			_actionRemoveFavorite->setEnabled(false);
		}
		_menu->exec(mapToGlobal(pos));
	}
}

void DAWorkFlowNodeListWidget::onActionAddFavoriteTriggered()
{
	DANodeMetaData md;
	if (DisplayInToolBox == getDisplayMode()) {
		DAToolBox* tb = getToolBox();
		md            = tb->getNodeMetaData(tb->mapFromGlobal(mapToGlobal(_lastCustoRequestedPoint)));
	} else {
		DANodeTreeWidget* tw = getTreeWidget();
		md                   = tw->getNodeMetaData(tw->mapFromGlobal(mapToGlobal(_lastCustoRequestedPoint)));
	}
	// 添加到fav
	if (md) {
		getToolBox()->addToFavorite(md);
		getTreeWidget()->addToFavorite(md);
	}
}

void DAWorkFlowNodeListWidget::onActionRemoveFavoriteTriggered()
{
	DANodeMetaData md;
	if (DisplayInToolBox == getDisplayMode()) {
		DAToolBox* tb = getToolBox();
		md            = tb->getNodeMetaData(tb->mapFromGlobal(mapToGlobal(_lastCustoRequestedPoint)));
	} else {
		DANodeTreeWidget* tw = getTreeWidget();
		md                   = tw->getNodeMetaData(tw->mapFromGlobal(mapToGlobal(_lastCustoRequestedPoint)));
	}
	if (md) {
		getToolBox()->removeFavorite(md);
		getTreeWidget()->removeFavorite(md);
	}
}

void DAWorkFlowNodeListWidget::onActionGroupTriggered(QAction* act)
{
	if (act == _actionViewNodeListByToolBox) {
		setDisplayMode(DisplayInToolBox);
	} else if (act == _actionViewNodeListByTree) {
		setDisplayMode(DisplayInTree);
	}
}

}  // end DA
