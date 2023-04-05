#ifndef DANODELISTWIDGET_H
#define DANODELISTWIDGET_H
#include <QListWidget>
#include <QListWidgetItem>
#include "DAGuiAPI.h"
#include "DANodeMetaData.h"
namespace DA
{
class DAGUI_API DANodeListWidgetItem : public QListWidgetItem
{
public:
    enum NodeItemType
    {
        ThisItemType = QListWidgetItem::UserType + 1
    };

public:
    DANodeListWidgetItem(const DANodeMetaData& node, QListWidget* listview = nullptr);
    DANodeMetaData getNodeMetaData() const;
    void setNodeMetaData(const DANodeMetaData& md);
};

/**
 * @brief 用于显示节点，并支持拖曳到view视图中
 */
class DAGUI_API DANodeListWidget : public QListWidget
{
    Q_OBJECT
public:
    DANodeListWidget(QWidget* p = nullptr);
    void addItems(const QList< DANodeMetaData >& nodeMetaDatas);
    void addItem(const DANodeMetaData& nodeMetaData);
    //通过位置获取对应的md
    DANodeMetaData getNodeMetaData(const QPoint& p) const;

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint _startPressPos;
};
}  // namespace DA
#endif  // FCNODELISTWIDGET_H
