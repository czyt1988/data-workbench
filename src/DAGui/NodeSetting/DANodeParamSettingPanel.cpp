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
#include "colorWidgets/SAColorToolButton.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
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
    DA_D(d);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    d->mPanel = new DAPropertyPanelContainerWidget(this);
    layout->addWidget(d->mPanel);
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

    // 3. 从DAPyNode读取参数值回写编辑器
    updateUI();
}

/**
 * @brief 获取内部属性面板容器
 */
DAPropertyPanelContainerWidget* DANodeParamSettingPanel::propertyPanel() const
{
    DA_DC(d);
    return d->mPanel;
}

/**
 * @brief 从DAPyNode读取参数值，回写所有编辑器
 *
 * 使用 QSignalBlocker 阻断本类的 propertyValueChanged 信号，
 * 防止回写编辑器时触发 onPropertyValueChanged 递归写入节点。
 * 遍历所有参数描述符，通过 DAPyNode::getParameterValue() 读取值并设置到对应编辑器。
 * 无效值直接跳过，不修改编辑器状态。
 */
void DANodeParamSettingPanel::updateUI()
{
    DA_D(d);
    if (!d->mPanel)
        return;

    // 阻断本类 propertyValueChanged 信号，防止回写触发 onPropertyValueChanged
    QSignalBlocker blocker(this);

    const auto& params = getParamDefs();
    DAPyNode& proxy    = node();
    int id             = 1;
    for (const auto& param : params) {
        DAPropertyItemWidget* item = d->mPanel->getPropertyItem(id);
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
                // 优先使用字典格式：{"family", "size", "bold", "italic", "color"}
                if (val.canConvert< QVariantMap >()) {
                    QVariantMap fontMap = val.toMap();
                    QFont f;
                    f.setFamily(fontMap.value("family").toString());
                    f.setPointSize(fontMap.value("size", 9).toInt());
                    f.setBold(fontMap.value("bold").toBool());
                    f.setItalic(fontMap.value("italic").toBool());
                    fe->setCurrentFont(f);
                    QColor c(fontMap.value("color").toString());
                    if (c.isValid()) {
                        fe->setCurrentFontColor(c);
                    }
                } else {
                    // 向后兼容：QFont::toString() 字符串格式
                    QFont f;
                    if (f.fromString(val.toString())) {
                        fe->setCurrentFont(f);
                    }
                }
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
    DA_D(d);
    if (!d->mPanel)
        return;

    d->mPanel->clearProperties();

    const auto& params = getParamDefs();

    // 无参数时显示占位标签
    if (params.isEmpty()) {
        auto* placeholder = new QLabel(QStringLiteral("无可配置参数"), d->mPanel);
        placeholder->setObjectName(QStringLiteral("da_placeholder_label"));
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setEnabled(false);
        d->mPanel->addProperty(0, placeholder);
    } else {
        // 创建类型注册表（构造时自动注册 11 种内置类型）
        DAParamTypeRegistry registry;

        int id = 1;
        for (const auto& param : params) {
            QString type    = param.typeLabel();
            QWidget* editor = registry.createEditor(type, param, d->mPanel);
            if (editor) {
                d->mPanel->addProperty(id, param.name(), param.description(), editor);
                connectEditorSignals(id, type, editor);
            }
            ++id;
        }
    }

    // 断开旧连接，防止 rebuild 时重复触发
    disconnect(d->mPanel,
               &DAPropertyPanelContainerWidget::propertyValueChanged,
               this,
               &DANodeParamSettingPanel::onPanelPropertyValueChanged);
    disconnect(this, &DANodeParamSettingPanel::propertyValueChanged, this, &DANodeParamSettingPanel::onPropertyValueChanged);

    // Hop-1: mPanel → onPanelPropertyValueChanged（转发信号）
    connect(d->mPanel,
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
 * @brief 3-hop 信号链第三跳：定向读取变更值并写入代理参数
 *
 * 根据 propertyId 定位对应的参数和编辑器，通过 readEditorValue() 读取值，
 * 再通过 DAPyNode::setParameterValue() 写入节点实例。
 *
 * @param propertyId 触发变更的属性ID
 */
void DANodeParamSettingPanel::onPropertyValueChanged(int propertyId)
{
    DA_D(d);
    DAPyNode& proxy = node();
    if (proxy.isNone())
        return;

    int idx            = propertyId - 1;
    const auto& params = getParamDefs();
    if (idx < 0 || idx >= params.size())
        return;

    DAPropertyItemWidget* item = d->mPanel->getPropertyItem(propertyId);
    if (!item)
        return;

    const auto& param = params[ idx ];
    QVariant val      = readEditorValue(item->editorWidget(), param.typeLabel());
    if (val.isValid()) {
        proxy.setParameterValue(param.name(), val);
    }
}

/**
 * @brief 从编辑器控件读取值（类型分发）
 *
 * 根据类型字符串对编辑器进行 qobject_cast，读取当前值并返回 QVariant。
 * 未知类型或 cast 失败时返回无效 QVariant。
 *
 * @param[in] editor 编辑器控件指针
 * @param[in] type 参数类型标签（int/float/bool/str/enum/file/folder/list/color/font/code）
 * @return 编辑器当前值，失败时返回无效 QVariant
 */
QVariant DANodeParamSettingPanel::readEditorValue(QWidget* editor, const QString& type)
{
    if (!editor || type.isEmpty())
        return { };

    if (type == "int") {
        auto* spin = qobject_cast< QSpinBox* >(editor);
        if (spin)
            return spin->value();
    } else if (type == "float") {
        auto* dsp = qobject_cast< QDoubleSpinBox* >(editor);
        if (dsp)
            return dsp->value();
    } else if (type == "bool") {
        auto* cb = qobject_cast< QCheckBox* >(editor);
        if (cb)
            return cb->isChecked();
    } else if (type == "str") {
        auto* le = qobject_cast< QLineEdit* >(editor);
        if (le)
            return le->text();
    } else if (type == "enum") {
        auto* combo = qobject_cast< QComboBox* >(editor);
        if (combo)
            return combo->currentText();
    } else if (type == "file" || type == "folder") {
        auto* fileEdit = qobject_cast< DA::DAFilePathEditWidget* >(editor);
        if (fileEdit)
            return fileEdit->getFilePath();
    } else if (type == "list") {
        QListWidget* listWidget = editor->findChild< QListWidget* >();
        if (listWidget) {
            QStringList strList;
            for (int i = 0; i < listWidget->count(); ++i) {
                strList.append(listWidget->item(i)->text());
            }
            return strList;
        }
    } else if (type == "color") {
        auto* btn = qobject_cast< DAColorPickerButton* >(editor);
        if (btn)
            return btn->color().name();
    } else if (type == "font") {
        auto* fe = qobject_cast< DAFontEditPannelWidget* >(editor);
        if (fe) {
            // 返回字典格式：{"family", "size", "bold", "italic", "color"}
            QFont f = fe->getCurrentFont();
            QVariantMap fontMap;
            fontMap[ "family" ] = f.family();
            fontMap[ "size" ]   = f.pointSize() > 0 ? f.pointSize() : 9;
            fontMap[ "bold" ]   = f.bold();
            fontMap[ "italic" ] = f.italic();
            fontMap[ "color" ]  = fe->getCurrentFontColor().name();
            return fontMap;
        }
    } else if (type == "code") {
        auto* codeEdit = qobject_cast< QPlainTextEdit* >(editor);
        if (codeEdit)
            return codeEdit->toPlainText();
    }

    return { };
}

/**
 * @brief 连接编辑器原生信号到 propertyValueChanged
 *
 * 根据参数类型，将编辑器控件的原生值变化信号连接到 emit propertyValueChanged(id)，
 * 使得用户修改编辑器后能触发 3-hop 信号链，最终将值写入节点代理。
 *
 * @param[in] id 属性ID
 * @param[in] type 参数类型标签
 * @param[in] editor 编辑器控件指针
 */
void DANodeParamSettingPanel::connectEditorSignals(int id, const QString& type, QWidget* editor)
{
    if (!editor)
        return;

    if (type == "int") {
        auto* spin = qobject_cast< QSpinBox* >(editor);
        if (spin) {
            connect(spin, QOverload< int >::of(&QSpinBox::valueChanged), this, [ this, id ](int) {
                emit propertyValueChanged(id);
            });
        }
    } else if (type == "float") {
        auto* dsp = qobject_cast< QDoubleSpinBox* >(editor);
        if (dsp) {
            connect(dsp, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, [ this, id ](double) {
                emit propertyValueChanged(id);
            });
        }
    } else if (type == "bool") {
        auto* cb = qobject_cast< QCheckBox* >(editor);
        if (cb) {
            connect(cb, &QCheckBox::toggled, this, [ this, id ](bool) { emit propertyValueChanged(id); });
        }
    } else if (type == "str") {
        auto* le = qobject_cast< QLineEdit* >(editor);
        if (le) {
            connect(le, &QLineEdit::textEdited, this, [ this, id ](const QString&) { emit propertyValueChanged(id); });
        }
    } else if (type == "enum") {
        auto* combo = qobject_cast< QComboBox* >(editor);
        if (combo) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [ this, id ](int) {
                emit propertyValueChanged(id);
            });
#else
            connect(combo, &QComboBox::currentIndexChanged, this, [ this, id ](int) { emit propertyValueChanged(id); });
#endif
        }
    } else if (type == "file" || type == "folder") {
        auto* fileEdit = qobject_cast< DA::DAFilePathEditWidget* >(editor);
        if (fileEdit) {
            connect(fileEdit, &DA::DAFilePathEditWidget::selectedPath, this, [ this, id ](const QString&) {
                emit propertyValueChanged(id);
            });
        }
    } else if (type == "color") {
        auto* btn = qobject_cast< DAColorPickerButton* >(editor);
        if (btn) {
            connect(btn, &SAColorToolButton::colorChanged, this, [ this, id ](const QColor&) {
                emit propertyValueChanged(id);
            });
        }
    } else if (type == "font") {
        auto* fe = qobject_cast< DAFontEditPannelWidget* >(editor);
        if (fe) {
            connect(fe, &DAFontEditPannelWidget::currentFontChanged, this, [ this, id ](const QFont&) {
                emit propertyValueChanged(id);
            });
            connect(fe, &DAFontEditPannelWidget::currentFontColorChanged, this, [ this, id ](const QColor&) {
                emit propertyValueChanged(id);
            });
        }
    } else if (type == "code") {
        auto* codeEdit = qobject_cast< QPlainTextEdit* >(editor);
        if (codeEdit) {
            connect(codeEdit, &QPlainTextEdit::textChanged, this, [ this, id ]() { emit propertyValueChanged(id); });
        }
    } else if (type == "list") {
        // list 编辑器是复合控件，连接内部的添加/删除按钮
        QList< QPushButton* > buttons = editor->findChildren< QPushButton* >();
        for (QPushButton* btn : buttons) {
            connect(btn, &QPushButton::clicked, this, [ this, id ]() { emit propertyValueChanged(id); });
        }
    }
}

/**
 * @brief 收集当前所有参数编辑器值，生成 QVariantHash
 *
 * 遍历所有参数代理，通过 readEditorValue() 从对应编辑器中读取值，
 * 组装为 QVariantHash 返回。未知类型跳过（不写入）。
 *
 * @return QVariantHash 对象，key 为参数名，value 为当前编辑器值
 */
QVariantHash DANodeParamSettingPanel::collectConfig() const
{
    DA_DC(d);
    QVariantHash config;
    if (!d->mPanel)
        return config;

    const auto& params = getParamDefs();
    int id             = 1;
    for (const auto& param : params) {
        QString name = param.name();
        QString type = param.typeLabel();

        if (name.isEmpty() || type.isEmpty()) {
            ++id;
            continue;
        }

        DAPropertyItemWidget* item = d->mPanel->getPropertyItem(id);
        if (!item) {
            ++id;
            continue;
        }

        QVariant val = readEditorValue(item->editorWidget(), type);
        if (val.isValid()) {
            config[ name ] = val;
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
