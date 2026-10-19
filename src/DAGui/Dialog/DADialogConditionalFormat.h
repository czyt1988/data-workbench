#ifndef DADIALOGCONDITIONALFORMAT_H
#define DADIALOGCONDITIONALFORMAT_H
#include "DAGuiAPI.h"
#include "DATableConditionalFormatRule.h"
#include <QDialog>
#include <QHash>
#include <QVector>
#include <QStringList>
#include <QList>

namespace Ui
{
class DADialogConditionalFormat;
}

namespace DA
{
class DATableConditionalFormatStore;

/**
 * @brief 条件格式规则编辑对话框（003 T1.5，一窗两栏）
 *
 * 左侧规则列表（按列分组显示、可上移/下移/启停/删除、底部清除所选列规则），
 * 右侧编辑面板（类型、列选择、条件与参数、颜色/字体、色阶端点、数据条颜色），
 * 布局参照 DADialogTableDisplayFormat 的「左列表 + 右 QStackedWidget」两栏先例。
 *
 * 实时预览（计划 WP3 步骤 1）：编辑期间把工作规则集经 store 的「预览规则集」叠加
 * 到表格（不落库、不入撤销栈），确定后由调用方按 resultRules() 与原值的差异提交
 * 命令入页撤销栈（D4），取消/关闭时预览还原。
 *
 * 惰性/只读数据源限制（§1.4-2）：setup 传 scaleBarAvailable=false 时
 * 色阶/数据条类型在下拉框置灰并显示说明文案，单元格规则正常提供。
 * 颜色选择用 Qt 标准 QColorDialog::getColor（父窗口传对话框自身）。
 */
class DAGUI_API DADialogConditionalFormat : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogConditionalFormat(DATableConditionalFormatStore* store, QWidget* parent = nullptr);
    ~DADialogConditionalFormat();

    // (re)初始化：columnNames=全部列名（位置索引对齐）；scopeColumns=本次编辑的列集合
    //（选区覆盖列）；scaleBarAvailable=false 时色阶/数据条置灰并给出说明
    void setup(const QStringList& columnNames, const QList< int >& scopeColumns, bool scaleBarAvailable);
    // 编辑结果（scope 列 → 规则列表，顺序即优先级；调用方 diff 后生成撤销命令）
    QHash< int, QVector< DATableConditionalFormatRule > > resultRules() const;

public Q_SLOTS:
    void accept() override;   // 确定：清预览，结果交调用方提交命令
    void reject() override;   // 取消：清预览还原

private Q_SLOTS:
    // 列表选择变化：装载新选中规则到编辑器
    void onRuleListCurrentChanged(int row);
    void onAddRule();
    void onRemoveRule();
    void onMoveUp();
    void onMoveDown();
    void onClearColumnRules();
    // 列下拉变化：把选中规则改派到新列（追加到目标列末尾）
    void onColumnComboChanged(int comboIndex);
    // 编辑器 → 工作规则集
    void onEditorChanged();

private:
    // 当前选中项对应的 (列, 规则索引)；无选中返回 false
    bool currentItemKey(int* column, int* index) const;
    // 重建左侧规则列表（保持选中项跟随 mEditCol/mEditIndex）
    void rebuildRuleList();
    // 装载规则到右侧编辑器（置 mUpdatingUi 守卫，同步 mEditCol/mEditIndex）
    void loadRuleToEditor(const DATableConditionalFormatRule& rule, int column, int index);
    // 把编辑器当前值写回工作集选中规则
    bool commitEditorToWorking();
    // 工作集 → store 预览叠加（实时预览）
    void refreshPreview();
    // 清除 store 预览（确定/取消/析构兜底）
    void clearStorePreview();
    // 按条件操作符刷新参数输入可用性（Between 双值 / IsEmpty 无值）
    void updateValueEditsEnabled();
    // 色块按钮图标刷新
    void updateSwatchButtons();
    // 列表/按钮可用性刷新
    void updateButtonStates();
    // 规则摘要文本（列表项显示）
    QString ruleSummary(int column, const DATableConditionalFormatRule& rule) const;
    // 颜色选择（QColorDialog::getColor，父窗口传对话框自身）；选中有效色返回 true
    bool pickColor(QColor* color);

private:
    Ui::DADialogConditionalFormat* ui;
    DATableConditionalFormatStore* mStore;  ///< 非拥有（预览叠加通道，可空）
    QStringList mColumnNames;               ///< 全部列名（位置索引对齐）
    QList< int > mScopeColumns;             ///< 本次编辑的列集合（升序）
    bool mScaleBarAvailable { true };       ///< 色阶/数据条是否可用（惰性/只读源为 false）
    QHash< int, QVector< DATableConditionalFormatRule > > mWorking;  ///< 工作规则集（编辑中，不落库）
    bool mUpdatingUi { false };             ///< 装载编辑器期间屏蔽控件信号
    int mEditCol { -1 };                    ///< 编辑器当前规则所属列（-1=无选中）
    int mEditIndex { -1 };                  ///< 编辑器当前规则在列内序号

    // ===== 编辑器颜色/字体暂存（勾选 valid 标志后才写入规则样式）=====
    QColor mSwatchBg { QColor(0xCE, 0x60, 0x43) };       ///< 背景色（默认官方橙红）
    QColor mSwatchFg { QColor(0x5D, 0x6D, 0x7E) };       ///< 前景色（默认官方深蓝灰）
    QFont mSwatchFont;                                    ///< 字体
    QColor mScaleMinColor { QColor(0xBD, 0xCD, 0xE8) };  ///< 色阶低端（官方浅蓝）
    QColor mScaleMidColor { QColor(0xFF, 0xFF, 0xFF) };  ///< 色阶中点（白）
    QColor mScaleMaxColor { QColor(0x52, 0x80, 0xC1) };  ///< 色阶高端（官方主蓝）
    QColor mBarColor { QColor(0xBD, 0xCD, 0xE8) };       ///< 数据条色（官方浅蓝）
};
}  // end of namespace DA
#endif  // DADIALOGCONDITIONALFORMAT_H
