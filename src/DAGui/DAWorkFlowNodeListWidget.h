#ifndef DAWORKFLOWNODELISTWIDGET_H
#define DAWORKFLOWNODELISTWIDGET_H
#include <QWidget>
#include <QMenu>
#include "DAGuiAPI.h"
#include "DANodeMetaData.h"
namespace Ui
{
class DAWorkFlowNodeListWidget;
}
class QDrag;
namespace DA
{
class DANodeTreeWidget;
class DAToolBox;
/**
 * @brief 工作流节点显示窗口
 */
class DAGUI_API DAWorkFlowNodeListWidget : public QWidget
{
	Q_OBJECT
public:
	/**
	 * @brief 设置节点的显示模式
	 */
	enum DisplayMode
	{
		DisplayInToolBox,  ///< 节点显示为抽屉箱
		DisplayInTree      ///< 节点显示为树
	};

public:
	DAWorkFlowNodeListWidget(QWidget* parent = nullptr);
	~DAWorkFlowNodeListWidget();
	// 添加节点
	void addItems(const QList< DANodeMetaData >& datas);
	// 设置显示模式
	void setDisplayMode(DisplayMode m);
	DisplayMode getDisplayMode() const;
	// 获取toolbox
	DAToolBox* getToolBox() const;
	// 获取TreeWidget
	DANodeTreeWidget* getTreeWidget() const;
	// 创建拖曳
	static QDrag* createDrag(QObject* parent, const DANodeMetaData& md);

private:
	// 构建菜单
	void buildMenu();
private slots:
	// 鼠标右键
	void onCustomContextMenuRequested(const QPoint& pos);
	// 添加收藏
	void onActionAddFavoriteTriggered();
	void onActionRemoveFavoriteTriggered();
	//
	void onActionGroupTriggered(QAction* act);

private:
	Ui::DAWorkFlowNodeListWidget* ui;
	QPoint _lastCustoRequestedPoint;
	QMenu* _menu;
	QAction* _actionViewNodeListByToolBox;
	QAction* _actionViewNodeListByTree;
	QActionGroup* _actionGroup;
	QAction* _actionAddFavorite;     ///< 加入为收藏
	QAction* _actionRemoveFavorite;  ///< 加入为收藏
};
}  // namespace DA
#endif  // DAWORKFLOWNODELISTWIDGET_H
