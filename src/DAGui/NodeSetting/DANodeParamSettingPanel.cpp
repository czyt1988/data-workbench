#include "DANodeParamSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAPropertyPanelWidget.h"
#include "DAParamTypeRegistry.h"
#include "DAPropertyItemWidget.h"
#include "QSpinBox"
#include "QDoubleSpinBox"
#include "QCheckBox"
#include "QLineEdit"
#include "QComboBox"
#include "QListWidget"
#include "QPlainTextEdit"
#include "DAFilePathEditWidget.h"
#include "DAColorPickerButton.h"
#include "DAFontEditPannelWidget.h"
#include <QVBoxLayout>
#include <QLabel>
namespace DA
{

class DANodeParamSettingPanel::PrivateData
{
    DA_DECLARE_PUBLIC(DANodeParamSettingPanel)

public:
    explicit PrivateData(DANodeParamSettingPanel* q) : q_ptr(q)
    {
    }

    DAPropertyPanelContainerWidget* mPanel = nullptr;
    QList< DAPyNodeParameter > mParameters;
    bool mBlockSignals = false;
};

/**
 * @brief 构造函数
 * @param parent 父控件
 * @note 不在构造时调用 buildPropertyPanel()，因为此时 getParamDefs() 为空。
 *       面板构建延迟到 setNode() 中参数列表填充后进行。
 */
DANodeParamSettingPanel::DANodeParamSettingPanel(QWidget* parent)
    : DAAbstractNodeSettingWidget(parent), DA_PIMPL_CONSTRUCT
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    d_func()->mPanel = new DAPropertyPanelContainerWidget(this);
    layout->addWidget(d_func()->mPanel);
}

/**
 * @brief 析构函数
 */
DANodeParamSettingPanel::~DANodeParamSettingPanel()
{
}

/**
 * @brief 设置节点代理并重建属性面板
 *
 * 覆盖基类 setNode()，在基类更新参数列表后重建属性面板并从代理加载参数值。
 * 流程：
 * 1. 调用基类 setNode() 更新参数代理列表
 * 2. 调用 buildPropertyPanel() 重建编辑器
 * 3. 调用 updateUI() 从 DAPyNodeParameter.value() 回写编辑器
 *
 * @param[in] proxy 节点代理常量引用
 */
void DANodeParamSettingPanel::setNode(const DAPyNode& proxy)
{
    // 1. 基类 setNode() 更新参数代理列表
    DAAbstractNodeSettingWidget::setNode(proxy);

    // 2. 重建面板
    buildPropertyPanel();

    // 3. 从DAPyNodeParameter读取参数值回写编辑器
    updateUI();
}

/**
 * @brief 获取内部属性面板容器
 */
DAPropertyPanelContainerWidget* DANodeParamSettingPanel::propertyPanel() const
{
    return d_func()->mPanel;
}

/**
 * @brief 从DAPyNode读取参数值，回写所有编辑器
 *
 * 使用 QSignalBlocker 阻断信号，防止回写触发递归的 onPropertyValueChanged。
 * 遍历所有参数描述符，通过 DAPyNode::getParameterValue() 读取值并设置到对应编辑器。
 * 无效值直接跳过，不修改编辑器状态。
 */
void DANodeParamSettingPanel::updateUI()
{
    auto* panel = d_func()->mPanel;
    if (!panel)
        return;

    QSignalBlocker blocker(panel);

    const auto& params = d_func()->mParameters;
    DAPyNode& proxy = node();
    int id             = 1;
    for (const auto& param : params) {
        DAPropertyItemWidget* item = panel->getPropertyItem(id);
        if (!item) {
            ++id;
            continue;
        }

        QWidget* editor = item->editorWidget();
        if (!editor) {
            ++id;
            continue;
        }

        QVariant val = proxy.getParameterValue(param.name());
        if (!val.isValid()) {
            ++id;
            continue;
        }

        QString type = param.typeLabel();

        if (type == "int") {
            auto* spin = qobject_cast< QSpinBox* >(editor);
            if (spin)
                spin->setValue(val.toInt());
        } else if (type == "float") {
            auto* dsp = qobject_cast< QDoubleSpinBox* >(editor);
            if (dsp)
                dsp->setValue(val.toDouble());
        } else if (type == "bool") {
            auto* cb = qobject_cast< QCheckBox* >(editor);
            if (cb)
                cb->setChecked(val.toBool());
        } else if (type == "str") {
            auto* le = qobject_cast< QLineEdit* >(editor);
            if (le)
                le->setText(val.toString());
        } else if (type == "enum") {
            auto* combo = qobject_cast< QComboBox* >(editor);
            if (combo) {
                int idx = combo->findText(val.toString());
                if (idx >= 0)
                    combo->setCurrentIndex(idx);
            }
        } else if (type == "file") {
            auto* fileEdit = qobject_cast< DA::DAFilePathEditWidget* >(editor);
            if (fileEdit)
                fileEdit->setFilePath(val.toString());
        } else if (type == "folder") {
            auto* foldEdit = qobject_cast< DA::DAFilePathEditWidget* >(editor);
            if (foldEdit)
                foldEdit->setFilePath(val.toString());
        } else if (type == "color") {
            auto* btn = qobject_cast< DAColorPickerButton* >(editor);
            if (btn)
                btn->setColor(QColor(val.toString()));
        } else if (type == "font") {
            auto* fe = qobject_cast< DAFontEditPannelWidget* >(editor);
            if (fe) {
                QFont f;
                f.fromString(val.toString());
                fe->setCurrentFont(f);
            }
        } else if (type == "code") {
            auto* codeEdit = qobject_cast< QPlainTextEdit* >(editor);
            if (codeEdit)
                codeEdit->setPlainText(val.toString());
        } else if (type == "list") {
            QListWidget* listWidget = editor->findChild< QListWidget* >();
            if (listWidget) {
                listWidget->clear();
                const QStringList strList = val.toStringList();
                for (const QString& s : strList) {
                    listWidget->addItem(s);
                }
            }
        }

        ++id;
    }
}

/**
 * @brief 构建属性面板
 *
 * 遍历参数描述符，通过 DAParamTypeRegistry 为每个参数动态创建编辑器控件，
 * 并添加到 DAPropertyPanelContainerWidget 中。
 * 无参数时显示"无可配置参数"占位标签。
 * 未知类型的参数跳过（编辑器为 nullptr），不会导致崩溃。
 *
 * 同时建立 3-hop 信号链：
 * - Hop-1: mPanel→onPanelPropertyValueChanged（转发信号）
 * - Hop-3: propertyValueChanged→onPropertyValueChanged（收集变更写入代理）
 *
 * @note 属性 ID 从 1 开始递增，与参数顺序对应
 */
void DANodeParamSettingPanel::buildPropertyPanel()
{
    auto* panel = d_func()->mPanel;
    if (!panel)
        return;

    panel->clearProperties();

    const auto& params = getParamDefs();

    // 无参数时显示占位标签
    if (params.isEmpty()) {
        auto* placeholder = new QLabel(QStringLiteral("无可配置参数"), panel);
        placeholder->setObjectName(QStringLiteral("da_placeholder_label"));
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setEnabled(false);
        panel->addProperty(0, placeholder);
    } else {
        // 复制参数描述符到私有数据，供 collectConfig/updateUI 使用
        d_func()->mParameters = params;

        // 创建类型注册表（构造时自动注册 11 种内置类型）
        DAParamTypeRegistry registry;

        int id = 1;
        for (const auto& param : d_func()->mParameters) {
            QWidget* editor = registry.createEditor(param.typeLabel(), param, panel);
            if (editor) {
                panel->addProperty(id, param.name(), param.description(), editor);
            }
            ++id;
        }
    }

    // 断开旧连接，防止 rebuild 时重复触发
    disconnect(panel,
               &DAPropertyPanelContainerWidget::propertyValueChanged,
               this,
               &DANodeParamSettingPanel::onPanelPropertyValueChanged);
    disconnect(this, &DANodeParamSettingPanel::propertyValueChanged, this, &DANodeParamSettingPanel::onPropertyValueChanged);

    // Hop-1: mPanel → onPanelPropertyValueChanged（转发信号）
    connect(panel,
            &DAPropertyPanelContainerWidget::propertyValueChanged,
            this,
            &DANodeParamSettingPanel::onPanelPropertyValueChanged);

    // Hop-3: propertyValueChanged → onPropertyValueChanged（收集变更写入代理）
    connect(this, &DANodeParamSettingPanel::propertyValueChanged, this, &DANodeParamSettingPanel::onPropertyValueChanged);
}

/**
 * @brief 3-hop 信号链第一跳
 */
void DANodeParamSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 3-hop 信号链第三跳：收集变更值写入代理参数
 *
 * 从编辑器收集当前参数值，通过 DAPyNode::setParameterValue() 逐个写入节点实例。
 *
 * @param propertyId 触发变更的属性ID（暂未用于定向更新，全量收集）
 */
void DANodeParamSettingPanel::onPropertyValueChanged(int propertyId)
{
    QVariantHash config = collectConfig();

    // 通过DAPyNode::setParameterValue()逐个写入Python节点实例
    DAPyNode& proxy = node();
    if (proxy.isNone()) {
        return;
    }
    for (auto it = config.constBegin(); it != config.constEnd(); ++it) {
        proxy.setParameterValue(it.key(), it.value());
    }
}

/**
 * @brief 收集当前所有参数编辑器值，生成 QVariantHash
 *
 * 遍历所有参数代理，按类型从对应编辑器中读取值，
 * 组装为 QVariantHash 返回。未知类型跳过（不写入）。
 *
 * @return QVariantHash 对象，key 为参数名，value 为当前编辑器值
 */
QVariantHash DANodeParamSettingPanel::collectConfig() const
{
    QVariantHash config;
    auto* panel = d_func()->mPanel;
    if (!panel)
        return config;

    const auto& params = d_func()->mParameters;
    int id             = 1;
    for (const auto& param : params) {
        QString name = param.name();
        QString type = param.typeLabel();

        if (name.isEmpty() || type.isEmpty()) {
            ++id;
            continue;
        }

        DAPropertyItemWidget* item = panel->getPropertyItem(id);
        if (!item) {
            ++id;
            continue;
        }

        QWidget* editor = item->editorWidget();
        if (!editor) {
            ++id;
            continue;
        }

        if (type == "int") {
            auto* spin = qobject_cast< QSpinBox* >(editor);
            if (spin)
                config[ name ] = spin->value();
        } else if (type == "float") {
            auto* dsp = qobject_cast< QDoubleSpinBox* >(editor);
            if (dsp)
                config[ name ] = dsp->value();
        } else if (type == "bool") {
            auto* cb = qobject_cast< QCheckBox* >(editor);
            if (cb)
                config[ name ] = cb->isChecked();
        } else if (type == "str") {
            auto* le = qobject_cast< QLineEdit* >(editor);
            if (le)
                config[ name ] = le->text();
        } else if (type == "enum") {
            auto* combo = qobject_cast< QComboBox* >(editor);
            if (combo)
                config[ name ] = combo->currentText();
        } else if (type == "file") {
            auto* fileEdit = qobject_cast< DA::DAFilePathEditWidget* >(editor);
            if (fileEdit)
                config[ name ] = fileEdit->getFilePath();
        } else if (type == "folder") {
            auto* foldEdit = qobject_cast< DA::DAFilePathEditWidget* >(editor);
            if (foldEdit)
                config[ name ] = foldEdit->getFilePath();
        } else if (type == "list") {
            // list 编辑器是复合控件（QWidget 容器），内部包含 QListWidget
            QListWidget* listWidget = editor->findChild< QListWidget* >();
            if (listWidget) {
                QStringList strList;
                for (int i = 0; i < listWidget->count(); ++i) {
                    strList.append(listWidget->item(i)->text());
                }
                config[ name ] = strList;
            }
        } else if (type == "color") {
            auto* btn = qobject_cast< DAColorPickerButton* >(editor);
            if (btn)
                config[ name ] = btn->color().name();
        } else if (type == "font") {
            auto* fe = qobject_cast< DAFontEditPannelWidget* >(editor);
            if (fe)
                config[ name ] = fe->getCurrentFont().toString();
        } else if (type == "code") {
            auto* codeEdit = qobject_cast< QPlainTextEdit* >(editor);
            if (codeEdit)
                config[ name ] = codeEdit->toPlainText();
        }

        ++id;
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
