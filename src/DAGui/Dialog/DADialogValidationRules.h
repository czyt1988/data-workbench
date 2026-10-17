#ifndef DADIALOGVALIDATIONRULES_H
#define DADIALOGVALIDATIONRULES_H
#include <QDialog>
#include <QList>
#include <QStringList>
#include "DAGuiAPI.h"
#include "DATableValidationRule.h"

class QListWidget;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QPlainTextEdit;
class QStackedWidget;
class QPushButton;
class QLabel;

namespace DA
{
/**
 * @brief 数据校验规则编辑对话框（005 T1.25，一窗两栏，同条件格式先例）
 *
 * 左侧规则列表（添加/删除/上移/下移），右侧编辑面板（列选择【列名 + 第 n 个同名列
 * 消歧，R20】、类型、级别、启用开关、按类型切换的参数编辑器）。编辑在对话框内的
 * 工作列表上进行，**不直接落库**；确定后由调用方（页面 widget）对比 store 现值，
 * 有差异才生成 DACommandValidationRule_setRules 入页撤销栈（D4），取消即丢弃。
 *
 * 参数录入约定：数值/日期区间上下界留空 = 不限（只设一端合法）；日期用
 * yyyy-MM-dd[ HH:mm:ss] 文本（解析失败标错并阻止确定）；枚举值一行一个；
 * 正则为整格完全匹配语义（fullmatch）。校验判定本身在 Python（da_validate_dataframe），
 * 对话框只做定义编辑，不预览违规（预览=执行校验，走「执行校验」入口）。
 */
class DAGUI_API DADialogValidationRules : public QDialog
{
    Q_OBJECT
public:
    // columnNames：全部列名（位置索引对齐，含重名）；rules：初始规则集
    explicit DADialogValidationRules(const QStringList& columnNames,
                                     const QList< DATableValidationRule >& rules,
                                     QWidget* parent = nullptr);
    ~DADialogValidationRules();

    // 编辑结果（调用方 diff 后生成撤销命令）
    QList< DATableValidationRule > resultRules() const;

public Q_SLOTS:
    void accept() override;  // 确定：校验编辑器合法性后返回结果

private Q_SLOTS:
    void onRuleListCurrentChanged(int row);
    void onAddRule();
    void onRemoveRule();
    void onMoveUp();
    void onMoveDown();
    void onEditorChanged();
    void onTypeChanged(int comboIndex);

private:
    void setupUI();
    void rebuildRuleList();
    // 装载规则到编辑器（置 mUpdatingUi 守卫）
    void loadRuleToEditor(const DATableValidationRule& rule);
    // 编辑器当前值写回工作集选中项；参数非法时输出错误并返回 false
    bool commitEditorToWorking(QString* err);
    // 编辑器 → 参数页切换与可用性
    void updateParamPages();
    void updateButtonStates();
    // 规则摘要文本（列表项显示：级别 + 列 + 类型 + 参数）
    QString ruleSummary(const DATableValidationRule& rule) const;
    // 列下拉当前选中的 (列名, occurrence)
    void currentColumnChoice(QString* name, int* occurrence) const;

    QStringList mColumnNames;                    ///< 全部列名（位置对齐，含重名）
    QList< DATableValidationRule > mWorking;     ///< 工作规则集（编辑中，不落库）
    int mEditIndex { -1 };                       ///< 当前编辑的规则下标（-1 无）
    bool mUpdatingUi { false };                  ///< 装载编辑器期间屏蔽控件信号

    QListWidget* mListRules { nullptr };
    QPushButton* mBtnAdd { nullptr };
    QPushButton* mBtnRemove { nullptr };
    QPushButton* mBtnUp { nullptr };
    QPushButton* mBtnDown { nullptr };
    QComboBox* mComboColumn { nullptr };
    QComboBox* mComboType { nullptr };
    QComboBox* mComboLevel { nullptr };
    QCheckBox* mCheckEnabled { nullptr };
    QLineEdit* mEditLabel { nullptr };
    QStackedWidget* mParamStack { nullptr };
    // 数值区间页
    QLineEdit* mEditNumMin { nullptr };
    QLineEdit* mEditNumMax { nullptr };
    // 日期区间页
    QLineEdit* mEditDateMin { nullptr };
    QLineEdit* mEditDateMax { nullptr };
    // 枚举页
    QPlainTextEdit* mEditEnumValues { nullptr };
    QCheckBox* mCheckEnumAllowOther { nullptr };
    // 正则页
    QLineEdit* mEditRegex { nullptr };
    QLabel* mLabelError { nullptr };
};
}  // end of namespace DA
#endif  // DADIALOGVALIDATIONRULES_H
