#ifndef DAPROPERTYFORMWIDGET_H
#define DAPROPERTYFORMWIDGET_H

#include "DACommonWidgetsAPI.h"
#include "DAFormRuleEvaluator.h"
#include "DAFormSpec.h"
#include <QHash>
#include <QStringList>
#include <QVariantMap>
#include <QWidget>

namespace DA
{
class DAFormEditorRegistry;
class DAPropertyItemWidget;
class DAPropertyPanelContainerWidget;

/**
 * @brief 通用属性表单宿主控件
 *
 * 基于 DAFormSpec 渲染表单：将顶层条目与分组中的字段通过 DAFormEditorRegistry 创建编辑器，
 * 挂载到 DAPropertyPanelContainerWidget 中，并在编辑器值变化时求值联动规则
 * （visibleWhen/enabledWhen/requiredWhen）刷新字段的可见性与启用状态。
 *
 * 该控件是 DAPropertyFormDialog 的主体，也可直接嵌入任意需要属性编辑的面板。
 *
 * @code
 * DAPropertyFormWidget* w = new DAPropertyFormWidget(parent);
 * w->setFormSpec(spec);
 * w->setValues(initialValues);
 * // 用户编辑后
 * QVariantMap result = w->values();
 * @endcode
 *
 * @see DAFormSpec, DAFormEditorRegistry, DAPropertyPanelContainerWidget
 */
class DACOMMONWIDGETS_API DAPropertyFormWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPropertyFormWidget)
public:
    explicit DAPropertyFormWidget(QWidget* parent = nullptr);
    ~DAPropertyFormWidget();

    // 设置表单规格，触发重建
    void setFormSpec(const DAFormSpec& spec);
    // 获取当前表单规格
    const DAFormSpec& formSpec() const;

    // 批量设置字段值（合并写入，阻塞反馈信号后重新求值规则）
    void setValues(const QVariantMap& values);
    // 收集所有字段的当前值
    QVariantMap values() const;
    // 获取指定字段的当前值（优先从编辑器实时读取）
    QVariant value(const QString& fieldName) const;
    // 设置单个字段值（委托 setValues）
    void setValue(const QString& fieldName, const QVariant& value);

Q_SIGNALS:
    /**
     * @brief 字段值变化信号
     *
     * 用户交互触发编辑器原生信号时发射，程序化 setValues 写入不触发此信号。
     * @param fieldName 字段名
     * @param value 字段当前值
     */
    void fieldValueChanged(const QString& fieldName, const QVariant& value);

private:
    void rebuildForm();
    void applyRules();
    void onFieldChanged(const QString& fieldName);
};
}  // namespace DA

#endif  // DAPROPERTYFORMWIDGET_H
