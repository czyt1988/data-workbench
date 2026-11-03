#ifndef DADIALOGVALIDATIONREPORT_H
#define DADIALOGVALIDATIONREPORT_H
#include <QDialog>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>
#include "DAGuiAPI.h"

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;

namespace DA
{
/**
 * @brief 数据校验结果报告对话框（005 T1.25）
 *
 * 按规则分组显示违规数与样例行；点击样例行发 locateCellRequested 交页面定位
 * （selectActualCell → showActualRow + 选中）。**非模态**（报告开着也能点定位看表格），
 * 调用方 show() + WA_DeleteOnClose。
 *
 * 导出走现成 DAMarkdownExporter（Markdown → HTML/PDF/Word，§4.3-5 两条现成路径
 * 取 Markdown 方向，不新增导出框架）；报告是只读产物。
 * 「清除高亮」只清 Source::Validation（来源隔离，不动查找命中，红线2）。
 */
class DAGUI_API DADialogValidationReport : public QDialog
{
    Q_OBJECT
public:
    // 报告条目（每条规则一项）
    struct Entry
    {
        QString ruleText;          ///< 规则摘要（已翻译组装：级别/列/类型/参数）
        bool isError { true };     ///< 级别（true=错误，false=警告）
        int columnPosition { -1 }; ///< 违规列位置（定位用；列缺失为 -1）
        bool columnMissing { false };  ///< 规则引用的列不存在
        QVector< int > rows;       ///< 违规行（数据源绝对行号，升序）
        QStringList rowTexts;      ///< 样例行显示文本（与 rows 对齐，调用方截断样例数）
    };

    explicit DADialogValidationReport(QWidget* parent = nullptr);
    ~DADialogValidationReport();

    // 装载结果并刷新树（evaluatedRuleCount=参与校验的启用规则数，elapsedMs=求值耗时）
    void setResults(const QList< Entry >& entries, int evaluatedRuleCount, int elapsedMs);
    // 违规总数（全部规则合计）
    int totalViolations() const;
    // 导出内容（Markdown 文本；单测直接断言，不经 WebEngine）
    QString buildMarkdown() const;

Q_SIGNALS:
    // 点击样例行定位（页面接：selectActualCell + 激活）
    void locateCellRequested(int actualRow, int columnPosition);
    // 清除违规高亮（页面接：overlay clear(Source::Validation)）
    void clearHighlightsRequested();

private Q_SLOTS:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onExport();
    void onClearHighlights();

private:
    void setupUI();
    void refreshSummary();

    QTreeWidget* mTree { nullptr };
    QLabel* mLabelSummary { nullptr };
    QPushButton* mBtnExport { nullptr };
    QPushButton* mBtnClear { nullptr };
    QList< Entry > mEntries;
    int mEvaluatedRuleCount { 0 };
    int mElapsedMs { 0 };
};
}  // end of namespace DA
#endif  // DADIALOGVALIDATIONREPORT_H
