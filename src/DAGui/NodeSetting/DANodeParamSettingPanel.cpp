#include "DANodeParamSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAParamTypeRegistry.h"
#include "QSpinBox"
#include "QDoubleSpinBox"
#include "QCheckBox"
#include "QLineEdit"
#include "QComboBox"
#include <QVBoxLayout>
#include <QLabel>
#include <QJsonArray>
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
    QJsonArray mParameters;
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
 */
void DANodeParamSettingPanel::testBuildPropertyPanelFromJson(const QJsonArray& params)
{
    d_func()->mParameters = params;
    auto* panel = d_func()->mPanel;
    if (!panel) return;

    panel->clearProperties();

    DAParamTypeRegistry registry;
    registry.registerDefaults();

    int currentId = 1;
    for (const auto& val : params) {
        QJsonObject desc = val.toObject();
        QString name = desc["name"].toString();
        QString type = desc["type"].toString();

        if (name.isEmpty() || type.isEmpty()) continue;

        QWidget* editor = registry.createEditor(type, desc, panel);
        if (editor) {
            panel->addProperty(currentId, name, desc["description"].toString(), editor);
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
        QJsonObject desc = val.toObject();
        QString name = desc["name"].toString();
        QString type = desc["type"].toString();

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
            }
        }
        ++currentId;
    }
    return config;
}

}  // namespace DA
