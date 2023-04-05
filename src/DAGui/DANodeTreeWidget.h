#ifndef DANODETREEWIDGET_H
#define DANODETREEWIDGET_H
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "DANodeMetaData.h"
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
    DANodeTreeWidgetItem(const DANodeMetaData& md);
    DANodeTreeWidgetItem(QTreeWidgetItem* parent, const DANodeMetaData& md);
    DANodeMetaData getNodeMetaData() const;
    void setNodeMetaData(const DANodeMetaData& md);
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
    void addItems(const QList< DANodeMetaData >& nodeMetaDatas);
    //逐个添加 注意此效率非常低
    void addItem(const DANodeMetaData& md);
    //添加到收藏
    void addToFavorite(const DANodeMetaData& md);
    void removeFavorite(const DANodeMetaData& md);
    //
    QTreeWidgetItem* getFavoriteItem();
    //创建收藏列
    QTreeWidgetItem* createFavoriteItem();
    //
    DANodeMetaData getNodeMetaData(const QPoint& p) const;

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint _startPressPos;
    QTreeWidgetItem* _favoriteItem;
};
}

#endif  // DANODETREEWIDGET_H
