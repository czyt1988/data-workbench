#ifndef DANODESETTINGWIDGET_H
#define DANODESETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include "DAPyNodeProxy.h"

namespace DA
{
class DAPropertyPanelContainerWidget;

/**
 * @brief 节点信息设置窗口
 *
 * 使用DAPropertyPanelContainerWidget展示节点元数据和名称属性，
 * 通过3-hop信号链处理属性变更。
 *
 * 属性列表：
 * - PID_Prototype: 限定名（只读）
 * - PID_Group: 节点组（只读）
 * - PID_Name: 节点名称（可编辑）
 *
 * @see DAPropertyPanelContainerWidget
 * @see DAPyNodeProxy
 */
class DAGUI_API DANodeSettingWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 属性ID枚举
     */
    enum PropertyId {
        PID_Prototype = 1,  ///< 限定名
        PID_Group,          ///< 节点组
        PID_Name            ///< 节点名称
    };

    explicit DANodeSettingWidget(QWidget* parent = nullptr);
    ~DANodeSettingWidget();

    // 设置节点
    void setNode(DAPyNodeProxy* p);

    // 获取当前节点
    DAPyNodeProxy* getNode() const;

    // 获取属性面板指针
    DAPropertyPanelContainerWidget* propertyPanel() const;

    // 刷新数据
    void updateData();

Q_SIGNALS:
    /**
     * @brief 属性值变化信号
     * @param propertyId 属性ID
     */
    void propertyValueChanged(int propertyId);

protected Q_SLOTS:
    // 构建属性面板
    void buildPropertyPanel();

    // 转发面板属性值变化信号
    void onPanelPropertyValueChanged(int propertyId);

    // 属性值变化处理
    void onPropertyValueChanged(int propertyId);

private:
    DAPropertyPanelContainerWidget* mPanel;
    DAPyNodeProxy* _nodePtr;
};
}  // end of namespace DA
#endif  // DANODESETTINGWIDGET_H
