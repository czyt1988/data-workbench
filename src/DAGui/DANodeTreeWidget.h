#ifndef DANODETREEWIDGET_H
#define DANODETREEWIDGET_H
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "DAPyNodeFactory.h"
#include "DAGuiAPI.h"

class QMouseEvent;
namespace DA
{

class DAGUI_API DANodeTreeWidgetItem : public QTreeWidgetItem
{
public:
    enum NodeItemType
    {
        ThisItemType = QTreeWidgetItem::UserType + 1
    };

public:
    DANodeTreeWidgetItem(const DAPyNodeMetaData& md);
    DANodeTreeWidgetItem(QTreeWidgetItem* parent, const DAPyNodeMetaData& md);
    DAPyNodeMetaData getNodeMetaData() const;
    void setNodeMetaData(const DAPyNodeMetaData& md);
};

/**
 * @brief 用于显示节点，并支持拖曳到view视图中
 */
class DAGUI_API DANodeTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    DANodeTreeWidget(QWidget* par = nullptr);
    //添加items
    void addItems(const QList< DAPyNodeMetaData >& nodeMetaDatas);
    //逐个添加 注意此效率非常低
    void addItem(const DAPyNodeMetaData& md);
    //添加到收藏
    void addToFavorite(const DAPyNodeMetaData& md);
    void removeFavorite(const DAPyNodeMetaData& md);
    //
    QTreeWidgetItem* getFavoriteItem();
    //创建收藏列
    QTreeWidgetItem* createFavoriteItem();
    //
    DAPyNodeMetaData getNodeMetaData(const QPoint& p) const;

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint _startPressPos;
    QTreeWidgetItem* _favoriteItem;
};
}

#endif  // DANODETREEWIDGET_H
