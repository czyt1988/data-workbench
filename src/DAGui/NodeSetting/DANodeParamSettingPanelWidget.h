#ifndef DANODEPARAMSETTINGPANELWIDGET_H
#define DANODEPARAMSETTINGPANELWIDGET_H

#include "DAGuiAPI.h"
#include "DAGlobals.h"
#include <QWidget>
#include <QHash>
#include <QString>
#include <QJsonObject>

class QStackedWidget;
class QLabel;

namespace DA
{

class DAPyNodeProxy;
class DANodeParamSettingPanel;

/**
 * @brief 节点参数设置面板调度器，基于 QStackedWidget + 惰性加载缓存
 *
 * 根据节点代理的 qualifiedName 懒加载对应参数设置面板，通过
 * DANodeParamSettingPanelFactory 创建面板实例，并使用 QHash 缓存已创建的面板，
 * 避免重复创建和销毁。
 *
 * 使用方式：
 * 1. 创建调度器实例，设置节点代理
 * 2. setNodeProxy(proxy) 自动获取描述符 → qualifiedName → 缓存查找 → 工厂创建 → 切换面板
 * 3. 面板值变化信号自动转发至外部
 * 4. null 代理显示占位标签 "未选中节点"
 *
 * @code
 * DANodeParamSettingPanelWidget* dispatcher = new DANodeParamSettingPanelWidget(parent);
 * dispatcher->setNodeProxy(myProxy);  // 自动创建并切换面板
 * dispatcher->setNodeProxy(nullptr);  // 显示占位标签
 * dispatcher->clearCache();           // 清除所有缓存面板
 * @endcode
 *
 * @see DANodeParamSettingPanel DANodeParamSettingPanelFactory DAPyNodeProxy
 */
class DAGUI_API DANodeParamSettingPanelWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DANodeParamSettingPanelWidget)

    friend class TestDANodeParamSettingPanelWidget;

public:
    explicit DANodeParamSettingPanelWidget(QWidget* parent = nullptr);
    ~DANodeParamSettingPanelWidget();

    // 设置节点代理（主入口 — 获取描述符 → qualifiedName → 缓存 → 创建 → 切换）
    void setNodeProxy(DAPyNodeProxy* proxy);

    // 清除所有缓存的面板
    void clearCache();

    // 获取当前显示的面板
    DANodeParamSettingPanel* currentPanel() const;

Q_SIGNALS:
    /**
     * @brief 参数值变化信号（转发自当前面板的 propertyValueChanged）
     * @param propertyId 属性ID
     */
    void propertyValueChanged(int propertyId);

protected:
    // 工厂无法创建面板时的默认面板创建方法
    DANodeParamSettingPanel* buildDefaultPanel();

    // === 测试辅助方法（仅在测试中使用）===

    // 使用模拟描述符测试调度逻辑（绕过 DAPyNodeProxy，直接指定 qualifiedName）
    void testSetNodeProxyWithDescriptor(const QJsonObject& descriptor);
};

}  // namespace DA

#endif  // DANODEPARAMSETTINGPANELWIDGET_H