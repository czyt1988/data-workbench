#include "DAPropertyFormWidget.h"
#include "DAGlobals.h"
#include "DAFormEditorRegistry.h"
#include "DAPropertyItemWidget.h"
#include "DAPropertyPanelContainerWidget.h"
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace DA
{
class DAPropertyFormWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAPropertyFormWidget)
public:
    PrivateData(DAPropertyFormWidget* p);
    void addField(const DAFormFieldDef& field, int id);

    DAPropertyPanelContainerWidget* panel { nullptr };
    DAFormEditorRegistry registry;
    DAFormSpec spec;
    QHash< QString, DAFormFieldDef > fieldDefs;   // 字段名 → 定义
    QHash< QString, QWidget* > editors;           // 字段名 → 编辑器
    QHash< QString, int > propertyIds;            // 字段名 → 面板属性ID
    QStringList fieldOrder;                       // 字段顺序（用于 values() 收集）
    QVariantMap currentValueMap;                  // 当前值集合（用于规则求值）
};

DAPropertyFormWidget::PrivateData::PrivateData(DAPropertyFormWidget* p) : q_ptr(p)
{
    registry.registerDefaults();
}

/**
 * @brief 将单个字段挂载到面板
 *
 * 创建编辑器、确定布局模式、注册到面板并建立值变化信号连接。
 * @param[in] field 字段定义
 * @param[in] id 分配的属性ID
 */
void DAPropertyFormWidget::PrivateData::addField(const DAFormFieldDef& field, int id)
{
    Q_Q(DAPropertyFormWidget);
    QWidget* editor = registry.createEditor(field, panel);
    if (!editor) {
        // 未知类型无法编辑，跳过
        return;
    }
    DAPropertyItemWidget::LayoutMode mode = (field.layout.compare("below", Qt::CaseInsensitive) == 0)
                                                ? DAPropertyItemWidget::BelowLayout
                                                : DAPropertyItemWidget::InlineLayout;
    panel->addProperty(id, field.name, field.description, editor, mode);

    fieldDefs[ field.name ]  = field;
    editors[ field.name ]    = editor;
    propertyIds[ field.name ] = id;
    fieldOrder.append(field.name);

    registry.connectValueChanged(field, editor, q, [ q, name = field.name ]() { q->onFieldChanged(name); });
}

/**
 * @brief 构造函数，初始化面板容器与注册表
 */
DAPropertyFormWidget::DAPropertyFormWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->panel = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->panel);
}

DAPropertyFormWidget::~DAPropertyFormWidget()
{
}

/**
 * @brief 设置表单规格并重建表单
 *
 * @param[in] spec 表单规格
 */
void DAPropertyFormWidget::setFormSpec(const DAFormSpec& spec)
{
    DA_D(d);
    d->spec = spec;
    rebuildForm();
}

/**
 * @brief 获取当前表单规格
 */
const DAFormSpec& DAPropertyFormWidget::formSpec() const
{
    DA_DC(d);
    return d->spec;
}

/**
 * @brief 重建表单
 *
 * 清空面板，按规格遍历顶层条目：分组条目展开为可折叠分组并依次添加子字段，
 * 字段条目直接添加。构建完成后读取各编辑器实时值填充 currentValueMap，
 * 再求值联动规则刷新可见性/启用状态。
 */
void DAPropertyFormWidget::rebuildForm()
{
    DA_D(d);
    d->panel->clearProperties();
    d->fieldDefs.clear();
    d->editors.clear();
    d->propertyIds.clear();
    d->fieldOrder.clear();
    d->currentValueMap.clear();

    int nextId = 1;
    for (const DAFormItemDef& item : std::as_const(d->spec.items)) {
        if (item.kind == DAFormItemDef::Group && item.group) {
            d->panel->addCollapsibleGroup(item.group->label);
            for (const DAFormItemDef& sub : std::as_const(item.group->items)) {
                if (sub.kind == DAFormItemDef::Field) {
                    d->addField(sub.field, nextId++);
                }
            }
            d->panel->endGroup();
        } else if (item.kind == DAFormItemDef::Field) {
            d->addField(item.field, nextId++);
        }
    }

    // 读取各编辑器实时值（已含适配器设置的默认值）填充当前值集合
    for (const QString& name : std::as_const(d->fieldOrder)) {
        const DAFormFieldDef& f = d->fieldDefs[ name ];
        QWidget* editor          = d->editors[ name ];
        QVariant v               = d->registry.readValue(f, editor);
        if (v.isValid()) {
            d->currentValueMap[ name ] = v;
        }
    }

    applyRules();
}

/**
 * @brief 字段值变化回调
 *
 * 读取编辑器实时值，更新当前值集合，发射 fieldValueChanged 信号，并重新求值规则。
 * @param[in] fieldName 字段名
 */
void DAPropertyFormWidget::onFieldChanged(const QString& fieldName)
{
    DA_D(d);
    auto it = d->editors.find(fieldName);
    if (it == d->editors.end()) {
        return;
    }
    const DAFormFieldDef& f = d->fieldDefs[ fieldName ];
    QVariant v               = d->registry.readValue(f, it.value());
    d->currentValueMap[ fieldName ] = v;
    Q_EMIT fieldValueChanged(fieldName, v);
    applyRules();
}

/**
 * @brief 求值联动规则并刷新字段可见性/启用状态
 *
 * 对每个字段调用 DAFormRuleEvaluator::evaluate，根据结果调用面板的
 * setPropertyVisible / setPropertyEnabled。required 状态当前无对应面板视觉，
 * 暂不处理（不阻塞用户）。
 */
void DAPropertyFormWidget::applyRules()
{
    DA_D(d);
    for (const QString& name : std::as_const(d->fieldOrder)) {
        const DAFormFieldDef& f = d->fieldDefs[ name ];
        DAFormFieldState state   = DAFormRuleEvaluator::evaluate(f, d->currentValueMap);
        int id                   = d->propertyIds.value(name, -1);
        if (id < 0) {
            continue;
        }
        d->panel->setPropertyVisible(id, state.visible);
        d->panel->setPropertyEnabled(id, state.enabled);
    }
}

/**
 * @brief 批量设置字段值
 *
 * 合并写入 currentValueMap，并同步到各编辑器。写入期间阻塞 this 信号与编辑器信号，
 * 避免触发 fieldValueChanged 反馈，随后重新求值规则。
 * @param[in] values 字段名 → 值
 */
void DAPropertyFormWidget::setValues(const QVariantMap& values)
{
    DA_D(d);
    QSignalBlocker blocker(this);
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        const QString& name = it.key();
        d->currentValueMap[ name ] = it.value();
        auto eIt = d->editors.find(name);
        if (eIt != d->editors.end()) {
            const DAFormFieldDef& f = d->fieldDefs[ name ];
            d->registry.writeValue(f, eIt.value(), it.value());
        }
    }
    applyRules();
}

/**
 * @brief 收集所有字段的当前值
 *
 * 按字段顺序从各编辑器实时读取，组装为 QVariantMap。
 * @return 字段名 → 当前值
 */
QVariantMap DAPropertyFormWidget::values() const
{
    DA_DC(d);
    QVariantMap result;
    for (const QString& name : std::as_const(d->fieldOrder)) {
        const DAFormFieldDef& f = d->fieldDefs[ name ];
        QWidget* editor          = d->editors[ name ];
        QVariant v               = d->registry.readValue(f, editor);
        if (v.isValid()) {
            result[ name ] = v;
        }
    }
    return result;
}

/**
 * @brief 获取指定字段的当前值
 *
 * 优先从编辑器实时读取，保证返回最新值；无编辑器时回退到当前值集合。
 * @param[in] fieldName 字段名
 * @return 字段当前值
 */
QVariant DAPropertyFormWidget::value(const QString& fieldName) const
{
    DA_DC(d);
    auto eIt = d->editors.find(fieldName);
    if (eIt != d->editors.end()) {
        const DAFormFieldDef& f = d->fieldDefs[ fieldName ];
        QVariant v               = d->registry.readValue(f, eIt.value());
        if (v.isValid()) {
            return v;
        }
    }
    return d->currentValueMap.value(fieldName);
}

/**
 * @brief 设置单个字段值
 *
 * 委托 setValues 完成实际写入。
 * @param[in] fieldName 字段名
 * @param[in] value 字段值
 */
void DAPropertyFormWidget::setValue(const QString& fieldName, const QVariant& value)
{
    setValues({ { fieldName, value } });
}

}  // namespace DA
