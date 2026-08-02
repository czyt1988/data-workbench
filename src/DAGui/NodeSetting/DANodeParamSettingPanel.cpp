#include "DANodeParamSettingPanel.h"
#include "DANodeParameterFormAdapter.h"
#include "DAPyNode.h"
#include <QLabel>
#include <QVBoxLayout>

namespace DA
{

class DANodeParamSettingPanel::PrivateData
{
    DA_DECLARE_PUBLIC(DANodeParamSettingPanel)

public:
    explicit PrivateData(DANodeParamSettingPanel* q) : q_ptr(q)
    {
    }

    DAPropertyFormWidget* mFormWidget      = nullptr;
    QLabel* mPlaceholderLabel              = nullptr;
};

/**
 * @brief 构造函数
 * @param parent 父控件
 * @note 创建 DAPropertyFormWidget 与空参数占位标签，并建立字段值变化的转发与回写连接。
 *       表单构建延迟到 setNode() 中参数列表填充后进行。
 */
DANodeParamSettingPanel::DANodeParamSettingPanel(QWidget* parent)
    : DAAbstractNodeSettingWidget(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    d->mFormWidget = new DAPropertyFormWidget(this);
    layout->addWidget(d->mFormWidget);

    // 空参数占位标签（默认隐藏）
    d->mPlaceholderLabel = new QLabel(QStringLiteral("无可配置参数"), this);
    d->mPlaceholderLabel->setObjectName(QStringLiteral("da_placeholder_label"));
    d->mPlaceholderLabel->setAlignment(Qt::AlignCenter);
    d->mPlaceholderLabel->setEnabled(false);
    d->mPlaceholderLabel->hide();
    layout->addWidget(d->mPlaceholderLabel);

    // 字段值变化：回写节点代理实例值（必须先于转发连接，确保外部监听者读到的是已写入的值）
    connect(d->mFormWidget, &DAPropertyFormWidget::fieldValueChanged, this, &DANodeParamSettingPanel::onFormFieldChanged);
    // 字段值变化：转发为本类信号（供 DANodeParamSettingPanelWidget 等外部监听）
    connect(d->mFormWidget, &DAPropertyFormWidget::fieldValueChanged, this, &DANodeParamSettingPanel::fieldValueChanged);
}

/**
 * @brief 析构函数
 */
DANodeParamSettingPanel::~DANodeParamSettingPanel()
{
}

/**
 * @brief 设置节点代理并重建表单
 *
 * 覆盖基类 setNode()，在基类更新参数代理列表后：
 * 1. 通过 DANodeParameterFormAdapter 将参数列表转换为 DAFormSpec
 * 2. 调用 DAPropertyFormWidget::setFormSpec 重建编辑器
 * 3. 调用 updateUI() 从节点实例回读参数值并填充编辑器
 *
 * 空参数或 isNone 代理时显示占位标签并隐藏表单。
 *
 * @param[in] proxy 节点代理常量引用
 */
void DANodeParamSettingPanel::setNode(const DAPyNode& proxy)
{
    // 1. 基类 setNode() 更新参数代理列表
    DAAbstractNodeSettingWidget::setNode(proxy);

    DA_D(d);

    // isNone 代理或无参数 → 显示占位标签
    if (proxy.isNone() || getParameters().isEmpty()) {
        d->mFormWidget->hide();
        d->mPlaceholderLabel->show();
        return;
    }

    // 2. 构建表单规格并重建表单
    DAFormSpec spec = DANodeParameterFormAdapter::toFormSpec(getParameters(), proxy.getNodeName());
    d->mFormWidget->setFormSpec(spec);

    d->mPlaceholderLabel->hide();
    d->mFormWidget->show();

    // 3. 从节点实例读取参数值填充编辑器
    updateUI();
}

/**
 * @brief 从节点实例读取参数值，回写所有编辑器
 *
 * 遍历参数列表，通过 DAPyNode::getParameterValue() 读取实例值并组装为 QVariantMap，
 * 调用 DAPropertyFormWidget::setValues 批量写入。setValues 内部以 QSignalBlocker 阻断
 * fieldValueChanged 反馈信号，无需额外 QSignalBlocker 即可避免递归回写。
 */
void DANodeParamSettingPanel::updateUI()
{
    DA_D(d);
    if (!d->mFormWidget || d->mFormWidget->isHidden()) {
        return;
    }

    DAPyNode& proxy = node();
    if (proxy.isNone()) {
        return;
    }

    QVariantMap values;
    const auto& params = getParameters();
    for (const DAPyNodeParameter& param : std::as_const(params)) {
        QVariant val = proxy.getParameterValue(param.name());
        if (val.isValid()) {
            values[ param.name() ] = val;
        }
    }
    d->mFormWidget->setValues(values);
}

/**
 * @brief 字段值变化回写节点代理
 *
 * 用户编辑触发 DAPropertyFormWidget::fieldValueChanged，此槽将新值通过
 * DAPyNode::setParameterValue() 写入节点实例。
 *
 * @param[in] fieldName 字段名（对应参数名）
 * @param[in] value 字段当前值
 */
void DANodeParamSettingPanel::onFormFieldChanged(const QString& fieldName, const QVariant& value)
{
    DAPyNode& proxy = node();
    if (proxy.isNone()) {
        return;
    }
    proxy.setParameterValue(fieldName, value);
}

/**
 * @brief 收集当前所有字段值，生成 QVariantHash
 *
 * 委托 DAPropertyFormWidget::values() 收集（QVariantMap），转换为 QVariantHash 返回。
 * key 为字段名（参数名），value 为当前编辑器值。
 *
 * @return QVariantHash 对象
 */
QVariantHash DANodeParamSettingPanel::collectConfig() const
{
    DA_DC(d);
    QVariantHash config;
    if (!d->mFormWidget) {
        return config;
    }
    const QVariantMap values = d->mFormWidget->values();
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        config[ it.key() ] = it.value();
    }
    return config;
}

/**
 * @brief 收集配置（测试用）
 *
 * 委托给 collectConfig() 实现，保持测试接口不变。
 */
QVariantHash DANodeParamSettingPanel::testCollectConfig() const
{
    return collectConfig();
}

}  // namespace DA
