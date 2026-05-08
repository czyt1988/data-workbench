#include "DANodeParamSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAPropertyPanelWidget.h"
#include "DAParamTypeRegistry.h"
#include "QSpinBox"
#include "QDoubleSpinBox"
#include "QCheckBox"
#include "QLineEdit"
#include "QComboBox"
#include "DAFilePathEditWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QJsonObject>

namespace DA
{

class DANodeParamSettingPanel::PrivateData
{
    DA_DECLARE_PUBLIC(DANodeParamSettingPanel)

public:
    explicit PrivateData(DANodeParamSettingPanel* q)
        : q_ptr(q)
    {
    }

    DAPropertyPanelContainerWidget* mPanel = nullptr;
    QVector< ParameterDescriptor > mParameters;
    bool mBlockSignals = false;
};

/**
 * @brief 构造函数
 * @param parent 父控件
 * @note stub - Task 5 完整实现时将替换此处
 */
DANodeParamSettingPanel::DANodeParamSettingPanel(QWidget* parent)
    : DAAbstractNodeSettingWidget(parent)
    , DA_PIMPL_CONSTRUCT
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    d_func()->mPanel = new DAPropertyPanelContainerWidget(this);
    layout->addWidget(d_func()->mPanel);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DANodeParamSettingPanel::~DANodeParamSettingPanel()
{
}

/**
 * @brief 获取内部属性面板容器
 */
DAPropertyPanelContainerWidget* DANodeParamSettingPanel::propertyPanel() const
{
    return d_func()->mPanel;
}

/**
 * @brief 更新 UI（stub — 使用 QSignalBlocker 阻断信号）
 */
void DANodeParamSettingPanel::updateUI()
{
    QSignalBlocker blocker(d_func()->mPanel);
    // TODO: Task 5 — 从代理读取配置
}

/**
 * @brief 构建属性面板（stub）
 */
void DANodeParamSettingPanel::buildPropertyPanel()
{
    auto* panel = d_func()->mPanel;
    if (!panel) return;

    panel->clearProperties();

    auto* placeholder = new QLabel(QStringLiteral("无可配置参数"), panel);
    placeholder->setObjectName(QStringLiteral("da_placeholder_label"));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setEnabled(false);
    panel->addProperty(0, placeholder);

    // 连接信号
    connect(panel, &DAPropertyPanelContainerWidget::propertyValueChanged,
            this, &DANodeParamSettingPanel::onPanelPropertyValueChanged);
}

/**
 * @brief 3-hop 信号链第一跳
 */
void DANodeParamSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 3-hop 信号链第三跳（stub）
 */
void DANodeParamSettingPanel::onPropertyValueChanged(int propertyId)
{
    // TODO: Task 5 — 收集变更值写入代理
}

/**
 * @brief 收集配置（stub）
 */
QJsonObject DANodeParamSettingPanel::collectConfig() const
{
    return QJsonObject();
}

/**
 * @brief 从 JSON 数组直接构建（测试用）
 *
 * 创建编辑器后，与便捷属性方法一致：直接连接编辑器 Widget 的信号到
 * rootPanel 的 propertyValueChanged，实现 3-hop 信号链第一跳。
 */
void DANodeParamSettingPanel::testBuildPropertyPanelFromJson(const QVector< ParameterDescriptor >& params)
{
    d_func()->mParameters = params;
    auto* panel = d_func()->mPanel;
    if (!panel) return;

    panel->clearProperties();

    DAParamTypeRegistry registry;
    registry.registerDefaults();
    DAPropertyPanelWidget* rootPanel = panel->rootPanel();

    int currentId = 1;
    for (const auto& val : params) {
        const ParameterDescriptor& desc = val;
        QString name = desc.name;
        QString type = desc.type;

        if (name.isEmpty() || type.isEmpty()) continue;

        QWidget* editor = registry.createEditor(type, desc.rawDescriptor, panel);
        if (editor) {
            panel->addProperty(currentId, name, desc.description, editor);

            // 与 DAPropertyPanelWidget 便捷属性方法一致：直接连接编辑器信号到 rootPanel
            if (type == "int") {
                auto* spinBox = qobject_cast<QSpinBox*>(editor);
                if (spinBox) {
                    connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                            rootPanel, [rootPanel, currentId](int) {
                                rootPanel->propertyValueChanged(currentId);
                            });
                }
            } else if (type == "float") {
                auto* dSpinBox = qobject_cast<QDoubleSpinBox*>(editor);
                if (dSpinBox) {
                    connect(dSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                            rootPanel, [rootPanel, currentId](double) {
                                rootPanel->propertyValueChanged(currentId);
                            });
                }
            } else if (type == "bool") {
                auto* checkBox = qobject_cast<QCheckBox*>(editor);
                if (checkBox) {
                    connect(checkBox, &QCheckBox::stateChanged,
                            rootPanel, [rootPanel, currentId](int) {
                                rootPanel->propertyValueChanged(currentId);
                            });
                }
            } else if (type == "str") {
                auto* lineEdit = qobject_cast<QLineEdit*>(editor);
                if (lineEdit) {
                    connect(lineEdit, &QLineEdit::textChanged,
                            rootPanel, [rootPanel, currentId](const QString&) {
                                rootPanel->propertyValueChanged(currentId);
                            });
                }
            } else if (type == "enum") {
                auto* combo = qobject_cast<QComboBox*>(editor);
                if (combo) {
                    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                            rootPanel, [rootPanel, currentId](int) {
                                rootPanel->propertyValueChanged(currentId);
                            });
                }
            } else if (type == "file" || type == "folder") {
                auto* fileEdit = qobject_cast<DAFilePathEditWidget*>(editor);
                if (fileEdit) {
                    connect(fileEdit, &DAFilePathEditWidget::selectedPath,
                            rootPanel, [rootPanel, currentId](const QString&) {
                                rootPanel->propertyValueChanged(currentId);
                            });
                }
            }
            ++currentId;
        }
    }

    if (currentId == 1) {
        auto* placeholder = new QLabel(QStringLiteral("无可配置参数"), panel);
        placeholder->setObjectName(QStringLiteral("da_placeholder_label"));
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setEnabled(false);
        panel->addProperty(0, placeholder);
    }

    connect(panel, &DAPropertyPanelContainerWidget::propertyValueChanged,
            this, &DANodeParamSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DANodeParamSettingPanel::propertyValueChanged,
            this, &DANodeParamSettingPanel::onPropertyValueChanged);
}

/**
 * @brief 收集配置（测试用）
 */
QJsonObject DANodeParamSettingPanel::testCollectConfig() const
{
    auto* panel = d_func()->mPanel;
    if (!panel) return QJsonObject();

    QJsonObject config;
    const auto& params = d_func()->mParameters;
    int currentId = 1;
    for (const auto& val : params) {
        const ParameterDescriptor& desc = val;
        QString name = desc.name;
        QString type = desc.type;

        if (name.isEmpty() || type.isEmpty()) continue;

        DAPropertyItemWidget* item = panel->getPropertyItem(currentId);
        if (item) {
            if (type == "int") {
                auto* spin = qobject_cast<QSpinBox*>(item->editorWidget());
                if (spin) config[name] = spin->value();
            } else if (type == "float") {
                auto* dsp = qobject_cast<QDoubleSpinBox*>(item->editorWidget());
                if (dsp) config[name] = dsp->value();
            } else if (type == "bool") {
                auto* cb = qobject_cast<QCheckBox*>(item->editorWidget());
                if (cb) config[name] = cb->isChecked();
            } else if (type == "str") {
                auto* le = qobject_cast<QLineEdit*>(item->editorWidget());
                if (le) config[name] = le->text();
            } else if (type == "enum") {
                auto* combo = qobject_cast<QComboBox*>(item->editorWidget());
                if (combo) config[name] = combo->currentText();
            } else if (type == "file") {
                auto* fileEdit = qobject_cast<DA::DAFilePathEditWidget*>(item->editorWidget());
                if (fileEdit) config[name] = fileEdit->getFilePath();
            } else if (type == "folder") {
                auto* foldEdit = qobject_cast<DA::DAFilePathEditWidget*>(item->editorWidget());
                if (foldEdit) config[name] = foldEdit->getFilePath();
            }
        }
        ++currentId;
    }
    return config;
}

}  // namespace DA
