#ifndef DADIALOGCONDITIONALFORMAT_H
#define DADIALOGCONDITIONALFORMAT_H
#include "DAGuiAPI.h"
#include "DATableConditionalFormatRule.h"
#include <QDialog>
#include <QHash>
#include <QVector>
#include <QStringList>
#include <QList>
#include <QColor>
#include <QFont>

class QComboBox;
class QLineEdit;
class QLabel;
class QCheckBox;
class QRadioButton;
class QPushButton;
class QListWidget;
class QStackedWidget;
class QDialogButtonBox;

namespace DA
{
class DATableConditionalFormatStore;
class DAItemListEditWidget;
class DAColorPickerButton;
class DAColorScalePreviewWidget;

/**
 * @brief 自定义条件格式对话框（规则管理 + 新建，渐进式自上而下）
 *
 * 交互顺序顺流而下（不左右跳）：
 * ① 规则类型（单元格规则/色阶/数据条）→
 * ② 应用列（DAItemListEditWidget：行内删除/菜单添加/搜索多选/移除所有）→
 * ③ 条件与格式（按类型切换的参数页：条件+值/色阶颜色+渐变预览/数据条颜色）→
 * ④ 现有规则列表（优先级顺序，上移/下移/删除/清除列；选中即装载到上方编辑）。
 *
 * 编辑语义：列表选中某规则时上方各节绑定该规则（改列即迁移/复制规则）；
 * 无选中时为「自由定义」，点「添加规则」把当前定义施加到所选各列。
 * 实时预览（store 预览规则集叠加，不落库不入撤销栈），确定后调用方按
 * resultRules() 与落库值差异提交命令入页撤销栈（D4），取消/关闭还原预览。
 * 惰性/只读数据源：色阶/数据条类型置灰并显示说明（§1.4-2）。
 */
class DAGUI_API DADialogConditionalFormat : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogConditionalFormat(DATableConditionalFormatStore* store, QWidget* parent = nullptr);
    ~DADialogConditionalFormat();

    // (re)初始化：columnNames=全部列名（位置索引对齐）；scopeColumns=本次编辑的列集合
    //（选区覆盖列，预填入列选择控件）；scaleBarAvailable=false 时色阶/数据条置灰并给出说明；
    // initialType 为 DATableConditionalFormatRule::Type 整数值（-1=不预设）
    void setup(const QStringList& columnNames, const QList< int >& scopeColumns, bool scaleBarAvailable,
               int initialType = -1);
    // 编辑结果（列 → 规则列表，顺序即优先级；调用方 diff 后生成撤销命令）
    QHash< int, QVector< DATableConditionalFormatRule > > resultRules() const;

public Q_SLOTS:
    void accept() override;  // 确定：清预览，结果交调用方提交命令
    void reject() override;  // 取消：清预览还原

private Q_SLOTS:
    // 列表选择变化：装载新选中规则到编辑器
    void onRuleListCurrentChanged(int row);
    void onAddRule();
    void onRemoveRule();
    void onMoveUp();
    void onMoveDown();
    void onClearColumnRules();
    // 列选择控件变化：绑定态迁移/复制规则到新的列集合
    void onColumnsChanged();
    // 编辑器 → 工作规则集
    void onEditorChanged();

private:
    void buildUi();    // 代码构建渐进式布局
    void connectUi();  // 信号接线
    // 当前选中项对应的 (列, 规则索引)；无选中返回 false
    bool currentItemKey(int* column, int* index) const;
    // 列选择控件当前选中的列索引列表
    QList< int > selectedColumns() const;
    // 重建规则列表（保持选中项跟随 mEditCol/mEditIndex）
    void rebuildRuleList();
    // 装载规则到编辑器（置 mUpdatingUi 守卫，同步 mEditCol/mEditIndex）
    void loadRuleToEditor(const DATableConditionalFormatRule& rule, int column, int index);
    // 编辑器当前值填入规则（类型/条件/值/样式/色阶/数据条）
    void fillRuleFromEditor(DATableConditionalFormatRule& rule) const;
    // 把编辑器当前值写回工作集选中规则
    bool commitEditorToWorking();
    // 工作集 → store 预览叠加（实时预览）
    void refreshPreview();
    // 清除 store 预览（确定/取消/析构兜底）
    void clearStorePreview();
    // 按条件操作符刷新参数输入可用性（Between 双值 / IsEmpty 系与重复系无值 / 日期系下拉）
    void updateValueEditsEnabled();
    // 色阶预览条刷新
    void updateScalePreview();
    // 列表/按钮可用性刷新
    void updateButtonStates();
    // 规则摘要文本（列表项显示）
    QString ruleSummary(int column, const DATableConditionalFormatRule& rule) const;
    // 发生日期类别显示名（与 DateOccurrence 枚举同序）
    QString dateOccurrenceName(int occ) const;

    DATableConditionalFormatStore* mStore;  ///< 非拥有（预览叠加通道，可空）
    QStringList mColumnNames;               ///< 全部列名（位置索引对齐）
    bool mScaleBarAvailable { true };       ///< 色阶/数据条是否可用（惰性/只读源为 false）
    QHash< int, QVector< DATableConditionalFormatRule > > mWorking;  ///< 工作规则集（编辑中，不落库）
    bool mUpdatingUi { false };             ///< 装载编辑器期间屏蔽控件信号
    int mEditCol { -1 };                    ///< 编辑器当前规则所属列（-1=自由定义态）
    int mEditIndex { -1 };                  ///< 编辑器当前规则在列内序号

    // ===== 控件（代码构建，自上而下四节）=====
    QComboBox* mComboType { nullptr };              ///< ① 规则类型
    DAItemListEditWidget* mColumnWidget { nullptr };  ///< ② 应用列
    QStackedWidget* mStackEditor { nullptr };       ///< ③ 条件与格式（按类型切页）
    // 单元格规则页
    QComboBox* mComboCondition { nullptr };
    QWidget* mRowDateOcc { nullptr };           ///< 发生日期行（容器，按条件显隐）
    QComboBox* mComboDateOcc { nullptr };       ///< 发生日期类别（DateOccurring 条件用）
    QWidget* mRowValue1 { nullptr };            ///< 条件值 1 行（容器，按条件显隐）
    QLabel* mLabelValue1 { nullptr };
    QLineEdit* mEditValue1 { nullptr };
    QWidget* mRowValue2 { nullptr };            ///< 条件值 2 行（容器，仅介于显示）
    QLabel* mLabelValue2 { nullptr };
    QLineEdit* mEditValue2 { nullptr };
    QCheckBox* mCheckUseBackground { nullptr };
    DAColorPickerButton* mBtnBackground { nullptr };
    QCheckBox* mCheckUseForeground { nullptr };
    DAColorPickerButton* mBtnForeground { nullptr };
    QCheckBox* mCheckUseFont { nullptr };
    QPushButton* mBtnFont { nullptr };
    // 色阶页
    QRadioButton* mRadioTwoColor { nullptr };
    QRadioButton* mRadioThreeColor { nullptr };
    DAColorPickerButton* mBtnScaleMin { nullptr };
    DAColorPickerButton* mBtnScaleMid { nullptr };
    DAColorPickerButton* mBtnScaleMax { nullptr };
    DAColorScalePreviewWidget* mScalePreview { nullptr };
    // 数据条页
    DAColorPickerButton* mBtnBarColor { nullptr };
    QCheckBox* mCheckBarGradient { nullptr };
    // ④ 现有规则
    QCheckBox* mCheckEnabled { nullptr };
    QListWidget* mListRules { nullptr };
    QPushButton* mBtnAdd { nullptr };
    QPushButton* mBtnRemove { nullptr };
    QPushButton* mBtnMoveUp { nullptr };
    QPushButton* mBtnMoveDown { nullptr };
    QPushButton* mBtnClearColumn { nullptr };
    QLabel* mLabelDisabledHint { nullptr };
    QDialogButtonBox* mButtonBox { nullptr };

    // ===== 编辑器字体暂存（勾选 valid 后才写入规则样式）=====
    QFont mSwatchFont;  ///< 字体
};
}  // end of namespace DA
#endif  // DADIALOGCONDITIONALFORMAT_H
