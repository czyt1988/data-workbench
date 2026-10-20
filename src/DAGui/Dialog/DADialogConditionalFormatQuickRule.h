#ifndef DADIALOGCONDITIONALFORMATQUICKRULE_H
#define DADIALOGCONDITIONALFORMATQUICKRULE_H
#include "DAGuiAPI.h"
#include "DAGlobals.h"
#include "DATableConditionalFormatRule.h"
#include <QDialog>

namespace DA
{
class DAColorPickerButton;

/**
 * @brief 突出显示单元格规则快速对话框（一键预设入口）
 *
 * 条件格式菜单「突出显示单元格规则」子菜单（大于/小于/介于/等于/文本包含）与
 * 「重复值」项唤出：确认条件值 → 选预设样式（或自定义背景/前景色）→ 确定，
 * 调用方经 resultRule() 取规则施加到选区列。界面自上而下渐进：
 * 条件说明 → 条件值（介于双值；重复/唯一为重复|唯一单选）→ 格式样式 → 自定义颜色。
 */
class DAGUI_API DADialogConditionalFormatQuickRule : public QDialog
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADialogConditionalFormatQuickRule)
public:
    // cond 为 CellRule 条件（Greater/Less/Between/Equal/Contains/Duplicate/Unique）；
    // Duplicate/Unique 时对话框提供「重复/唯一」单选切换（cond 为初始值）
    explicit DADialogConditionalFormatQuickRule(DATableConditionalFormatRule::Condition cond,
                                                  QWidget* parent = nullptr);
    ~DADialogConditionalFormatQuickRule();

    // 确定后的规则（CellRule：条件 + 条件值 + 样式）
    DATableConditionalFormatRule resultRule() const;

public Q_SLOTS:
    void accept() override;  // 确定：校验条件值非空（需要值的条件）

private:
    // 按条件刷新标题/值输入可用性/重复单选可见性
    void updateConditionUi();
    // 样式下拉变化：自定义时显示颜色行
    void updateStyleUi();
    // 预设样式显示名（与 DAConditionalFormatPresets::highlightStyles 同序）
    QStringList highlightStyleNames() const;

    DATableConditionalFormatRule::Condition mCondition;  ///< 条件（重复/唯一可经单选切换）
};
}  // end of namespace DA
#endif  // DADIALOGCONDITIONALFORMATQUICKRULE_H
