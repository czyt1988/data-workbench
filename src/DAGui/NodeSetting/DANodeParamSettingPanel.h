#ifndef DANODEPARAMSETTINGPANEL_H
#define DANODEPARAMSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAAbstractNodeSettingWidget.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAGlobals.h"
#include "DAPyWorkFlow/DAPyNodeParameter.h"
#include <QVariantHash>
#include <QList>

class QLabel;

namespace DA
{

/**
 * @brief 参数面板中间层，基于 SceneB 模式构建
 *
 * 继承 DAAbstractNodeSettingWidget，内部持有 DAPropertyPanelContainerWidget，
 * 通过 DAParamTypeRegistry 动态创建各类型参数编辑器，
 * 实现 3-hop 信号链和 updateUI 信号阻断。
 *
 * 使用方式：
 * 1. 创建面板实例
 * 2. 调用 setNode(proxy) 设置节点代理，自动重建属性面板并初始化缓存
 * 3. 编辑器值变化触发 3-hop 信号链 → 代理更新
 * 4. updateUI() 从缓存读取配置，使用 QSignalBlocker 阻断回写信号
 *
 * @code
 * DANodeParamSettingPanel* panel = new DANodeParamSettingPanel(parent);
 * panel->setNode(myProxy);  // 自动 rebuild + 初始化缓存
 * @endcode
 *
 * @see DAAbstractNodeSettingWidget DAPropertyPanelContainerWidget DAParamTypeRegistry
 */
class DAGUI_API DANodeParamSettingPanel : public DAAbstractNodeSettingWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DANodeParamSettingPanel)

    friend class TestNodeParamSettingPanel;

public:
    explicit DANodeParamSettingPanel(QWidget* parent = nullptr);
    ~DANodeParamSettingPanel();

    // 覆盖 setNode：基类更新参数列表后重建属性面板并初始化缓存
    void setNode(const DAPyNode& proxy) override;

    // 获取属性面板容器（子类可通过此指针调用便捷方法）
    DAPropertyPanelContainerWidget* propertyPanel() const;

    // 实现 DAAbstractNodeSettingWidget 的 updateUI
    void updateUI() override;

    // 收集当前所有参数编辑器值 → 生成 QVariantHash 配置
    QVariantHash collectConfig() const;

Q_SIGNALS:
    /**
     * @brief 参数值变化信号（转发自 DAPropertyPanelContainerWidget）
     * @param propertyId 属性ID
     * @note 3-hop 信号链第二跳：onPanelPropertyValueChanged → emit 此信号
     */
    void propertyValueChanged(int propertyId);

protected Q_SLOTS:
    // 构建属性面板（遍历参数描述符 → 注册编辑器），setNode() 中调用
    void buildPropertyPanel();

    // 3-hop 信号链第一跳：转发 mPanel 的 propertyValueChanged
    void onPanelPropertyValueChanged(int propertyId);

    // 3-hop 信号链第三跳：收集变更值 → 写入代理配置
    void onPropertyValueChanged(int propertyId);

protected:
    // 收集配置（测试暴露）
    QVariantHash testCollectConfig() const;
};

}  // namespace DA

#endif  // DANODEPARAMSETTINGPANEL_H
