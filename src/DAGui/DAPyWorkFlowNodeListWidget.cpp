#include "DAPyWorkFlowNodeListWidget.h"
#include <QActionGroup>
#include <QDrag>
#include "ui_DAPyWorkFlowNodeListWidget.h"
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
// DAPyWorkFlowNodeListWidget
//===================================================
DAPyWorkFlowNodeListWidget::DAPyWorkFlowNodeListWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAPyWorkFlowNodeListWidget), mMenu(nullptr)
{
	ui->setupUi(this);
	mActionViewNodeListByToolBox = new QAction(this);
	mActionViewNodeListByToolBox->setObjectName("actionViewDataListByTable");
	mActionViewNodeListByToolBox->setCheckable(true);
	mActionViewNodeListByToolBox->setIcon(QIcon(":/DAGui/icon/showDataInList.svg"));
	mActionViewNodeListByTree = new QAction(this);
	mActionViewNodeListByTree->setObjectName("actionViewDataListByTree");
	mActionViewNodeListByTree->setCheckable(true);
	mActionViewNodeListByTree->setIcon(QIcon(":/DAGui/icon/showDataInTree.svg"));
	mActionGroup = new QActionGroup(this);
	mActionGroup->addAction(mActionViewNodeListByToolBox);
	mActionGroup->addAction(mActionViewNodeListByTree);
	mActionGroup->setExclusive(true);
	ui->toolButtonList->setDefaultAction(mActionViewNodeListByToolBox);
	ui->toolButtonTree->setDefaultAction(mActionViewNodeListByTree);
	setDisplayMode(DisplayInToolBox);
	connect(this,
			&DAPyWorkFlowNodeListWidget::customContextMenuRequested,
			this,
			&DAPyWorkFlowNodeListWidget::onCustomContextMenuRequested);
	connect(mActionGroup, &QActionGroup::triggered, this, &DAPyWorkFlowNodeListWidget::onActionGroupTriggered);
}

DAPyWorkFlowNodeListWidget::~DAPyWorkFlowNodeListWidget()
{
    delete ui;
}

/**
 * @brief 添加节点，并自动分组
 * @param datas
 */
void DAPyWorkFlowNodeListWidget::addItems(const QList< DAPyNodeMetaData >& datas)
{
	ui->workFlowToolBox->addItems(datas);
	ui->workflowTreeWidget->addItems(datas);
}

/**
 * @brief 重建节点列表
 *
 * 先清空 toolbox 与树的节点分组（收藏保留），再按新数据重建。
 * 插件热插拔导致节点元数据变化后由宿主调用刷新
 * @param datas
 */
void DAPyWorkFlowNodeListWidget::updateItems(const QList< DAPyNodeMetaData >& datas)
{
	ui->workFlowToolBox->clear();
	ui->workflowTreeWidget->clearNodes();
	addItems(datas);
}

/**
 * @brief 设置显示模式
 * @param m
 */
void DAPyWorkFlowNodeListWidget::setDisplayMode(DAPyWorkFlowNodeListWidget::DisplayMode m)
{
	switch (m) {
	case DisplayInToolBox:
		mActionViewNodeListByToolBox->setChecked(true);
		ui->stackedWidget->setCurrentWidget(ui->workFlowToolBox);
		break;
	default:
		mActionViewNodeListByTree->setChecked(true);
		ui->stackedWidget->setCurrentWidget(ui->workflowTreeWidget);
		break;
	}
}

/**
 * @brief 获取当前的显示模式
 * @return
 */
DAPyWorkFlowNodeListWidget::DisplayMode DAPyWorkFlowNodeListWidget::getDisplayMode() const
{
	if (ui->stackedWidget->currentWidget() == ui->workFlowToolBox) {
		return DisplayInToolBox;
	}
	return DisplayInTree;
}

DAToolBox* DAPyWorkFlowNodeListWidget::getToolBox() const
{
	return ui->workFlowToolBox;
}

DANodeTreeWidget* DAPyWorkFlowNodeListWidget::getTreeWidget() const
{
    return ui->workflowTreeWidget;
}

/**
 * @brief 创建拖曳
 * @param parent
 * @param md
 * @return
 */
QDrag* DAPyWorkFlowNodeListWidget::createDrag(QObject* parent, const DAPyNodeMetaData& md)
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
void DAPyWorkFlowNodeListWidget::buildMenu()
{
	mMenu                 = new QMenu(this);
	mActionAddFavorite    = new QAction(QIcon(":/DAGui/icon/favorite.svg"), tr("Favorite"), this);  // cn:收藏
	mActionRemoveFavorite = new QAction(QIcon(":/DAGui/icon/removeFavorite.svg"), tr("Remove Favorite"), this);  // cn:移除收藏
	mMenu->addAction(mActionAddFavorite);
	mMenu->addAction(mActionRemoveFavorite);
	connect(mActionAddFavorite, &QAction::triggered, this, &DAPyWorkFlowNodeListWidget::onActionAddFavoriteTriggered);
	connect(mActionRemoveFavorite, &QAction::triggered, this, &DAPyWorkFlowNodeListWidget::onActionRemoveFavoriteTriggered);
}

/**
 * @brief 鼠标右键
 * @param pos
 */
void DAPyWorkFlowNodeListWidget::onCustomContextMenuRequested(const QPoint& pos)
{
	if (!mMenu) {
		buildMenu();
	}
	mLastCustoRequestedPoint = pos;
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
			mActionAddFavorite->setEnabled(false);
			mActionRemoveFavorite->setEnabled(true);
		} else {
			mActionAddFavorite->setEnabled(true);
			mActionRemoveFavorite->setEnabled(false);
		}
		mMenu->exec(mapToGlobal(pos));
	}
}

void DAPyWorkFlowNodeListWidget::onActionAddFavoriteTriggered()
{
	DAPyNodeMetaData md;
	if (DisplayInToolBox == getDisplayMode()) {
		DAToolBox* tb = getToolBox();
		md            = tb->getNodeMetaData(tb->mapFromGlobal(mapToGlobal(mLastCustoRequestedPoint)));
	} else {
		DANodeTreeWidget* tw = getTreeWidget();
		md                   = tw->getNodeMetaData(tw->mapFromGlobal(mapToGlobal(mLastCustoRequestedPoint)));
	}
	// 添加到fav
	if (md.isValid()) {
		getToolBox()->addToFavorite(md);
		getTreeWidget()->addToFavorite(md);
	}
}

void DAPyWorkFlowNodeListWidget::onActionRemoveFavoriteTriggered()
{
	DAPyNodeMetaData md;
	if (DisplayInToolBox == getDisplayMode()) {
		DAToolBox* tb = getToolBox();
		md            = tb->getNodeMetaData(tb->mapFromGlobal(mapToGlobal(mLastCustoRequestedPoint)));
	} else {
		DANodeTreeWidget* tw = getTreeWidget();
		md                   = tw->getNodeMetaData(tw->mapFromGlobal(mapToGlobal(mLastCustoRequestedPoint)));
	}
	if (md.isValid()) {
		getToolBox()->removeFavorite(md);
		getTreeWidget()->removeFavorite(md);
	}
}

void DAPyWorkFlowNodeListWidget::onActionGroupTriggered(QAction* act)
{
	if (act == mActionViewNodeListByToolBox) {
		setDisplayMode(DisplayInToolBox);
	} else if (act == mActionViewNodeListByTree) {
		setDisplayMode(DisplayInTree);
	}
}

}  // end DA
