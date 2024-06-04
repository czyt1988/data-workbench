#ifndef DAGRAPHICSPIXMAPITEMSETTINGWIDGET_H
#define DAGRAPHICSPIXMAPITEMSETTINGWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
namespace Ui
{
class DAGraphicsPixmapItemSettingWidget;
}

namespace DA
{
class DAGraphicsPixmapItem;

class DAGUI_API DAGraphicsPixmapItemSettingWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsPixmapItemSettingWidget)
public:
    explicit DAGraphicsPixmapItemSettingWidget(QWidget* parent = nullptr);
    ~DAGraphicsPixmapItemSettingWidget();
    // 设置需要配置的item
    void setItem(DAGraphicsPixmapItem* item);
    // 获取维护的item
    DAGraphicsPixmapItem* getItem() const;
    // 更新
    void updateData();
    // 设置当前的alpha值,注意这个函数不会发射信号
    void setCurrentAlphaValue(int v);
    int getCurrentAlphaValue() const;
signals:
    /**
     * @brief 图片的透明度改变信号
     * @param v
     */
    void pixmapAlphaValueChanged(int v);
private slots:
    // 滑竿控件改变
    void onHorizontalSliderAlphaValueChanged(int v);
    // 数值控件改变
    void onSpinBoxValueChanged(int v);

private:
    Ui::DAGraphicsPixmapItemSettingWidget* ui;
};
}

#endif  // DAGRAPHICSPIXMAPITEMSETTINGWIDGET_H
