#ifndef DANODELINKITEMSETTINGWIDGET_H
#define DANODELINKITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPen>
#include "DAAbstractNodeLinkGraphicsItem.h"
namespace Ui
{
class DANodeLinkItemSettingWidget;
}
namespace DA
{
class DANodeGraphicsScene;
/**
 * @brief 链接设置
 */
class DAGUI_API DANodeLinkItemSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DANodeLinkItemSettingWidget(QWidget* parent = nullptr);
    ~DANodeLinkItemSettingWidget();
    //设置连线样式，此函数不发射信号
    void setCurrentLinkLineStyle(DAAbstractNodeLinkGraphicsItem::LinkLineStyle s, bool updateLinkItem = false);
    //设置画笔
    void setLinkLinePen(const QPen& p, bool updateLinkItem = false);
    //设置item
    void setLinkItem(DAAbstractNodeLinkGraphicsItem* link);
    DAAbstractNodeLinkGraphicsItem* getLinkItem() const;
    //设置端点的信息
    void updateLinkEndpointInfo(DAAbstractNodeLinkGraphicsItem* link);
    //刷新数据,此函数不触发信号
    void updateData();
    //设置了DAGraphicsSceneWithUndoStack 能实现redo/undo
    void setScene(DANodeGraphicsScene* sc);

protected:
    void initEndpointComboxBox();
    QPixmap generateEndPointPixmap(DAAbstractNodeLinkGraphicsItem* link, DAAbstractNodeLinkGraphicsItem::EndPointType epType);
signals:
    /**
     * @brief 连线样式改变发射的信号
     * @param s
     */
    void currentLinkLineStyleChanged(DAAbstractNodeLinkGraphicsItem::LinkLineStyle s);

    /**
     * @brief 请求连线的画笔改变
     * @param p
     */
    void linkLinePenChanged(const QPen& p);
private slots:
    void onComboBoxLinkStyleCurrentIndexChanged(int index);
    void onLinkLinePenChanged(const QPen& p);
    void onNodeLinksRemoved(const QList< DAAbstractNodeLinkGraphicsItem* >& items);
    void onSpinBoxEndpointSizeValueChanged(int arg1);
    void onComboBoxFrontStyleCurrentIndexChanged(int index);
    void onComboBoxEndStyleCurrentIndexChanged(int index);

private:
    Ui::DANodeLinkItemSettingWidget* ui;
    DAAbstractNodeLinkGraphicsItem* _linkItem;
    DANodeGraphicsScene* _scene;
    QSize _endpointIconSize;
};
}
#endif  // DANODELINKITEMSETTINGWIDGET_H
