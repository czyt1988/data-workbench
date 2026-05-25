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
#include <QJsonObject>
#include <QJsonArray>
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
    QVector< DAParamDef > mParameters;
    bool mBlockSignals = false;
    QJsonObject mConfigCache;
};

/**
 * @brief 构造函数
 * @param parent 父控件
 * @note stub - Task 5 完整实现时将替换此处
 */
DANodeParamSettingPanel::DANodeParamSettingPanel(QWidget* parent)
    : DAAbstractNodeSettingWidget(parent), DA_PIMPL_CONSTRUCT
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
 * @brief 从代理缓存读取配置，回写所有编辑器值
 *
 * 使用 QSignalBlocker 阻断信号，防止回写触发递归的 onPropertyValueChanged。
 * 遍历所有参数描述符，按类型从 mConfigCache 读取值设置到对应编辑器。
 * 缺失的 key 直接跳过，不修改编辑器状态。
 */
void DANodeParamSettingPanel::updateUI()
{
    auto* panel = d_func()->mPanel;
    if (!panel)
        return;

    QSignalBlocker blocker(panel);

    const auto& config = d_func()->mConfigCache;
    const auto& params = d_func()->mParameters;
    int id             = 1;
    for (const auto& desc : params) {
        QString key = desc.name;
        if (!config.contains(key)) {
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

        QJsonValue val = config.value(key);
        QString type   = desc.type;

        if (type == "int") {
            auto* spin = qobject_cast< QSpinBox* >(editor);
            if (spin && val.isDouble())
                spin->setValue(val.toInt());
        } else if (type == "float") {
            auto* dsp = qobject_cast< QDoubleSpinBox* >(editor);
            if (dsp && val.isDouble())
                dsp->setValue(val.toDouble());
        } else if (type == "bool") {
            auto* cb = qobject_cast< QCheckBox* >(editor);
            if (cb && val.isBool())
                cb->setChecked(val.toBool());
        } else if (type == "str") {
            auto* le = qobject_cast< QLineEdit* >(editor);
            if (le && val.isString())
                le->setText(val.toString());
        } else if (type == "enum") {
            auto* combo = qobject_cast< QComboBox* >(editor);
            if (combo && val.isString()) {
                int idx = combo->findText(val.toString());
                if (idx >= 0)
                    combo->setCurrentIndex(idx);
            }
        } else if (type == "file") {
            auto* fileEdit = qobject_cast< DA::DAFilePathEditWidget* >(editor);
            if (fileEdit && val.isString())
                fileEdit->setFilePath(val.toString());
        } else if (type == "folder") {
            auto* foldEdit = qobject_cast< DA::DAFilePathEditWidget* >(editor);
            if (foldEdit && val.isString())
                foldEdit->setFilePath(val.toString());
        } else if (type == "color") {
            auto* btn = qobject_cast< DAColorPickerButton* >(editor);
            if (btn && val.isString())
                btn->setColor(QColor(val.toString()));
        } else if (type == "font") {
            auto* fe = qobject_cast< DAFontEditPannelWidget* >(editor);
            if (fe && val.isString()) {
                QFont f;
                f.fromString(val.toString());
                fe->setCurrentFont(f);
            }
        } else if (type == "code") {
            auto* codeEdit = qobject_cast< QPlainTextEdit* >(editor);
            if (codeEdit && val.isString())
                codeEdit->setPlainText(val.toString());
        } else if (type == "list") {
            QListWidget* listWidget = editor->findChild< QListWidget* >();
            if (listWidget && val.isArray()) {
                listWidget->clear();
                QJsonArray arr = val.toArray();
                for (const auto& elem : arr) {
                    listWidget->addItem(elem.toString());
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
        // 复制参数描述符到私有数据，填充 propertyId 供 collectConfig 使用
        d_func()->mParameters = params;

        // 创建类型注册表（构造时自动注册 11 种内置类型）
        DAParamTypeRegistry registry;

        int id = 1;
        for (auto& desc : d_func()->mParameters) {
            desc.propertyId = id;
            QWidget* editor = registry.createEditor(desc.type, desc, panel);
            if (editor) {
                panel->addProperty(id, desc.name, desc.description, editor);
            }
            // 未知类型：editor == nullptr，跳过，不崩溃
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
 * @brief 3-hop 信号链第三跳：收集变更值写入代理配置
 *
 * 从编辑器收集当前配置 → 缓存到 mConfigCache → 调用 proxy->setConfig() 写入代理。
 * 若代理为空则仅更新本地缓存，不执行 Python 写入。
 *
 * @param propertyId 触发变更的属性ID（暂未用于定向更新，全量收集）
 */
void DANodeParamSettingPanel::onPropertyValueChanged(int propertyId)
{
    QJsonObject config     = collectConfig();
    d_func()->mConfigCache = config;

    DAPyNode* proxy = getNodeProxy();
    if (proxy) {
        proxy->setConfig(config);
    }
}

/**
 * @brief 收集当前所有参数编辑器值，生成 QJsonObject 配置
 *
 * 遍历所有参数描述符，按类型从对应编辑器中读取值，
 * 组装为 QJsonObject 返回。未知类型跳过（返回 QJsonValue()）。
 *
 * @return QJsonObject 配置对象，key 为参数名，value 为当前编辑器值
 */
QJsonObject DANodeParamSettingPanel::collectConfig() const
{
    QJsonObject config;
    auto* panel = d_func()->mPanel;
    if (!panel)
        return config;

    const auto& params = d_func()->mParameters;
    int id             = 1;
    for (const auto& desc : params) {
        QString name = desc.name;
        QString type = desc.type;

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
                QJsonArray arr;
                for (int i = 0; i < listWidget->count(); ++i) {
                    arr.append(listWidget->item(i)->text());
                }
                config[ name ] = arr;
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
        } else {
            // 未知类型：跳过，返回 QJsonValue()（即 null）
            config[ name ] = QJsonValue();
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
QJsonObject DANodeParamSettingPanel::testCollectConfig() const
{
    return collectConfig();
}

}  // namespace DA
