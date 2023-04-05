#ifndef DANODEITEMSETTINGWIDGET_H
#define DANODEITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QSizeF>
namespace Ui
{
class DANodeItemSettingWidget;
}

namespace DA
{
DA_IMPL_FORWARD_DECL(DANodeItemSettingWidget)
class DANodeGraphicsScene;
class DAGraphicsResizeableItem;
class DAAbstractNodeGraphicsItem;
class DAGUI_API DANodeItemSettingWidget : public QWidget
{
    Q_OBJECT
    DA_IMPL(DANodeItemSettingWidget)
public:
    explicit DANodeItemSettingWidget(QWidget* parent = nullptr);
    ~DANodeItemSettingWidget();
    //设置需要配置的item
    void setItem(DAGraphicsResizeableItem* item);
    //获取维护的item
    DAGraphicsResizeableItem* getItem() const;
    //设置了DAGraphicsSceneWithUndoStack 能实现redo/undo
    void setScene(DANodeGraphicsScene* sc);
    //更新
    void updateData();
    //更新位置信息
    void updatePosition();
    //更新旋转信息
    void updateRotation();
    //更新body信息
    void updateBodySize();
    //更新item的状态
    void updateItemState();
    //更新连接点的位置
    void updateLinkPointLocation();
private slots:
    void onDoubleSpinBoxBodyWidthValueChanged(double v);
    void onDoubleSpinBoxBodyHeightValueChanged(double v);
    void onDoubleSpinBoxRotationValueChanged(double v);
    void onDoubleSpinBoxXValueChanged(double v);
    void onDoubleSpinBoxYValueChanged(double v);
    void onCheckBoxMovableStateChanged(int state);
    void onCheckBoxResizableStateChanged(int state);
    void onNodeItemsRemoved(const QList< DA::DAAbstractNodeGraphicsItem* >& items);
    void onActionGroupTriggered(QAction* a);

private:
    void resetValue();

private:
    Ui::DANodeItemSettingWidget* ui;
};
}
#endif  // DANODEITEMSETTINGWIDGET_H
