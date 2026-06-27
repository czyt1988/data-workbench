#ifndef DANODEPARAMSETTINGPANEL_H
#define DANODEPARAMSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAAbstractNodeSettingWidget.h"
#include "DAPropertyFormWidget.h"
#include "DAGlobals.h"
#include "DAPyWorkFlow/DAPyNodeParameter.h"
#include <QVariantHash>
#include <QList>

namespace DA
{

/**
 * @brief 节点参数面板，基于通用 DAPropertyFormWidget 构建
 *
 * 继承 DAAbstractNodeSettingWidget，内部持有 DAPropertyFormWidget。
 * setNode() 时通过 DANodeParameterFormAdapter 将 DAPyNodeParameter 列表转换为 DAFormSpec，
 * 交由 DAPropertyFormWidget（配合 DAFormEditorRegistry）完成编辑器的创建/读取/写入/信号连接，
 * 取代旧版 DAParamTypeRegistry + DAPropertyPanelContainerWidget + 3-hop 信号链的方案。
 *
 * 字段值变化通过 DAPropertyFormWidget::fieldValueChanged 直接写回节点代理，
 * updateUI() 则从节点代理回读并 setValues（setValues 内部阻断反馈信号，无需额外 QSignalBlocker）。
 *
 * @code
 * DANodeParamSettingPanel* panel = new DANodeParamSettingPanel(parent);
 * panel->setNode(myProxy);  // 重建表单 + 加载实例值
 * @endcode
 *
 * @see DAAbstractNodeSettingWidget DAPropertyFormWidget DANodeParameterFormAdapter
 */
class DAGUI_API DANodeParamSettingPanel : public DAAbstractNodeSettingWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DANodeParamSettingPanel)

    friend class TestNodeParamSettingPanel;

public:
    explicit DANodeParamSettingPanel(QWidget* parent = nullptr);
    ~DANodeParamSettingPanel();

    // 覆盖 setNode：基类更新参数列表后重建表单并加载实例值
    void setNode(const DAPyNode& proxy) override;

    // 实现 DAAbstractNodeSettingWidget 的 updateUI
    void updateUI() override;

    // 收集当前所有字段值 → 生成 QVariantHash 配置
    QVariantHash collectConfig() const;

Q_SIGNALS:
    /**
     * @brief 字段值变化信号（转发自 DAPropertyFormWidget）
     * @param fieldName 字段名（对应参数名）
     * @param value 字段当前值
     */
    void fieldValueChanged(const QString& fieldName, const QVariant& value);

protected Q_SLOTS:
    // 用户编辑字段后写回节点代理
    void onFormFieldChanged(const QString& fieldName, const QVariant& value);

protected:
    // 收集配置（测试暴露）
    QVariantHash testCollectConfig() const;
};

}  // namespace DA

#endif  // DANODEPARAMSETTINGPANEL_H
