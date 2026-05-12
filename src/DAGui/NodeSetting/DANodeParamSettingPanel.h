#ifndef DANODEPARAMSETTINGPANEL_H
#define DANODEPARAMSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAAbstractNodeSettingWidget.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAGlobals.h"
#include "DAPyWorkFlow/DAParameterDescriptor.h"
#include "DANodeDescriptor.h"
#include <QJsonObject>
#include <QVector>

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
 * 1. 创建面板实例，设置 nodeProxy 或直接使用描述符
 * 2. buildPropertyPanel() 在构造时自动调用，遍历参数描述符创建编辑器
 * 3. 编辑器值变化触发 3-hop 信号链 → 代理更新
 * 4. updateUI() 从代理读取配置，使用 QSignalBlocker 阻断回写信号
 *
 * @code
 * DANodeParamSettingPanel* panel = new DANodeParamSettingPanel(parent);
 * panel->setNodeProxy(myProxy);  // 自动 rebuild
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

    // 获取属性面板容器（子类可通过此指针调用便捷方法）
    DAPropertyPanelContainerWidget* propertyPanel() const;

    // 实现 DAAbstractNodeSettingWidget 的 updateUI
    void updateUI() override;

Q_SIGNALS:
    /**
     * @brief 参数值变化信号（转发自 DAPropertyPanelContainerWidget）
     * @param propertyId 属性ID
     * @note 3-hop 信号链第二跳：onPanelPropertyValueChanged → emit 此信号
     */
    void propertyValueChanged(int propertyId);

protected Q_SLOTS:
    // 构建属性面板（遍历参数描述符 → 注册编辑器），构造时自动调用
    void buildPropertyPanel();

    // 3-hop 信号链第一跳：转发 mPanel 的 propertyValueChanged
    void onPanelPropertyValueChanged(int propertyId);

    // 3-hop 信号链第三跳：收集变更值 → 写入代理配置
    void onPropertyValueChanged(int propertyId);

protected:
    // 收集当前所有参数编辑器值 → 生成 QJsonObject 配置
    QJsonObject collectConfig() const;

    // 收集配置（测试暴露）
    QJsonObject testCollectConfig() const;
};

}  // namespace DA

#endif  // DANODEPARAMSETTINGPANEL_H
