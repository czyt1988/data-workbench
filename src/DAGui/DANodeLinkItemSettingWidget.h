#ifndef DANODELINKITEMSETTINGWIDGET_H
#define DANODELINKITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPen>
#include "DAPyLinkGraphicsItem.h"
class QComboBox;

namespace DA
{
class DAPyWorkFlowGraphicsScene;
class DAPropertyPanelContainerWidget;
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
    void setCurrentLinkLineStyle(DAGraphicsLinkItem::LinkLineStyle s, bool updateLinkItem = false);
    //设置画笔
    void setLinkLinePen(const QPen& p, bool updateLinkItem = false);
    //设置item
    void setLinkItem(DAPyLinkGraphicsItem* link);
    DAPyLinkGraphicsItem* getLinkItem() const;
    //设置端点的信息
    void updateLinkEndpointInfo(DAPyLinkGraphicsItem* link);
    //刷新数据,此函数不触发信号
    void updateData();
    //设置了DAGraphicsSceneWithUndoStack 能实现redo/undo
    void setScene(DAPyWorkFlowGraphicsScene* sc);

protected:
    void initEndpointComboxBox();
    QPixmap generateEndPointPixmap(DAPyLinkGraphicsItem* link, DAGraphicsLinkItem::EndPointType epType);
signals:
    /**
     * @brief 连线样式改变发射的信号
     * @param s
     */
    void currentLinkLineStyleChanged(DAGraphicsLinkItem::LinkLineStyle s);

    /**
     * @brief 请求连线的画笔改变
     * @param p
     */
    void linkLinePenChanged(const QPen& p);
private slots:
    void onPropertyValueChanged(int propertyId);
    void onComboBoxFrontStyleCurrentIndexChanged(int index);
    void onComboBoxEndStyleCurrentIndexChanged(int index);
    void onNodeLinksRemoved(const QList< DAPyLinkGraphicsItem* >& items);

private:
    // 属性ID定义
    enum PropertyId {
        PropertyPen = 1,
        PropertyLinkStyle = 2,
        PropertyEndpointSize = 3,
        PropertyFrontStyle = 4,
        PropertyEndStyle = 5
    };
    DAPropertyPanelContainerWidget* mPanel;
    QComboBox* _comboBoxFrontStyle;
    QComboBox* _comboBoxEndStyle;
    DAPyLinkGraphicsItem* _linkItem;
    DAPyWorkFlowGraphicsScene* _scene;
    QSize _endpointIconSize;
};
}
#endif  // DANODELINKITEMSETTINGWIDGET_H