#include "DADialogValidationReport.h"
#include "MarkdownView/DAMarkdownExporter.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QFileInfo>
#include <QDialogButtonBox>

namespace DA
{

/**
 * @brief 构造（非模态，调用方 show()+WA_DeleteOnClose）
 * @param parent 父窗口
 */
DADialogValidationReport::DADialogValidationReport(QWidget* parent) : QDialog(parent)
{
    setupUI();
}

DADialogValidationReport::~DADialogValidationReport()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）：汇总标签 + 分组树 + 导出/清除/关闭
 */
void DADialogValidationReport::setupUI()
{
    setWindowTitle(tr("Validation Report"));  // cn:数据校验报告
    QVBoxLayout* main = new QVBoxLayout(this);
    mLabelSummary     = new QLabel(this);
    mLabelSummary->setObjectName(QStringLiteral("validationReportSummary"));
    main->addWidget(mLabelSummary);
    mTree = new QTreeWidget(this);
    mTree->setObjectName(QStringLiteral("validationReportTree"));
    mTree->setColumnCount(1);
    mTree->setHeaderHidden(true);
    mTree->setRootIsDecorated(true);
    main->addWidget(mTree, 1);
    QHBoxLayout* btns = new QHBoxLayout();
    mBtnExport        = new QPushButton(tr("Export..."), this);  // cn:导出...
    mBtnExport->setObjectName(QStringLiteral("btnExportReport"));
    mBtnClear         = new QPushButton(tr("Clear Highlights"), this);  // cn:清除高亮
    mBtnClear->setObjectName(QStringLiteral("btnClearHighlights"));
    QPushButton* btnClose = new QPushButton(tr("Close"), this);         // cn:关闭
    btnClose->setObjectName(QStringLiteral("btnCloseReport"));
    btns->addWidget(mBtnExport);
    btns->addWidget(mBtnClear);
    btns->addStretch();
    btns->addWidget(btnClose);
    main->addLayout(btns);
    connect(mTree, &QTreeWidget::itemClicked, this, &DADialogValidationReport::onItemClicked);
    connect(mBtnExport, &QPushButton::clicked, this, &DADialogValidationReport::onExport);
    connect(mBtnClear, &QPushButton::clicked, this, &DADialogValidationReport::onClearHighlights);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::close);
    resize(560, 480);
}

/**
 * @brief 装载结果并重建分组树
 *
 * 顶层 = 规则（错误橙红 / 警告金黄前缀标记，官方色板语义色），子项 = 样例行。
 * 列缺失的规则单独标注（无法定位）。违规行以数据存进 item 的 UserRole 供点击定位。
 */
void DADialogValidationReport::setResults(const QList< Entry >& entries, int evaluatedRuleCount, int elapsedMs)
{
    mEntries            = entries;
    mEvaluatedRuleCount = evaluatedRuleCount;
    mElapsedMs          = elapsedMs;
    mTree->clear();
    for (const Entry& e : std::as_const(mEntries)) {
        const QString prefix = e.isError ? QStringLiteral("● ") : QStringLiteral("▲ ");
        QString headText     = prefix + e.ruleText;
        if (e.columnMissing) {
            headText += tr("  [column missing]");  // cn:  [列不存在]
        } else {
            headText += tr("  —  %1 violation(s)").arg(e.rows.size());  // cn:  —  %1 处违规
            if (e.rows.size() > e.rowTexts.size()) {
                headText += tr(" (showing first %1)").arg(e.rowTexts.size());  // cn:（显示前 %1 条）
            }
        }
        QTreeWidgetItem* top = new QTreeWidgetItem(mTree);
        top->setText(0, headText);
        top->setForeground(0, e.isError ? QColor(0xCE, 0x60, 0x43)   // 官方橙红（错误，DESIGN.md §2）
                                        : QColor(0xB8, 0x86, 0x00));  // 金黄加深保证文字可读（警告语义 #E6C27C 系）
        top->setData(0, Qt::UserRole, -1);  // 顶层不可定位
        if (!e.columnMissing) {
            for (int i = 0; i < e.rows.size() && i < e.rowTexts.size(); ++i) {
                QTreeWidgetItem* child = new QTreeWidgetItem(top);
                child->setText(0, e.rowTexts[ i ]);
                child->setData(0, Qt::UserRole, e.rows[ i ]);
                child->setData(0, Qt::UserRole + 1, e.columnPosition);
            }
        }
    }
    mTree->expandAll();
    refreshSummary();
}

/**
 * @brief 汇总标签：X 条规则、Y 处违规、耗时
 */
void DADialogValidationReport::refreshSummary()
{
    const int total = totalViolations();
    mLabelSummary->setText(tr("Evaluated %1 rule(s), found %2 violation(s), took %3 ms")
                               .arg(mEvaluatedRuleCount)
                               .arg(total)
                               .arg(mElapsedMs));  // cn:校验 %1 条规则，发现 %2 处违规，耗时 %3 毫秒
    mBtnExport->setEnabled(!mEntries.isEmpty());
}

int DADialogValidationReport::totalViolations() const
{
    int total = 0;
    for (const Entry& e : std::as_const(mEntries)) {
        total += e.rows.size();
    }
    return total;
}

/**
 * @brief 点击样例行 → 定位到表格单元格（顶层项 column=-1 忽略）
 */
void DADialogValidationReport::onItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (!item) {
        return;
    }
    const int row = item->data(0, Qt::UserRole).toInt();
    if (row < 0) {
        return;  // 顶层规则项
    }
    const int colPos = item->data(0, Qt::UserRole + 1).toInt();
    Q_EMIT locateCellRequested(row, colPos);
}

/**
 * @brief 导出内容（Markdown 文本，按规则分组 + 样例行；单测直接断言）
 */
QString DADialogValidationReport::buildMarkdown() const
{
    QString md;
    md += QStringLiteral("# ") + tr("Validation Report") + QStringLiteral("\n\n");  // cn:数据校验报告
    md += tr("Evaluated %1 rule(s), found %2 violation(s), took %3 ms").arg(mEvaluatedRuleCount).arg(totalViolations()).arg(mElapsedMs);
    md += QStringLiteral("\n\n");
    for (const Entry& e : std::as_const(mEntries)) {
        const QString level = e.isError ? tr("Error") : tr("Warning");  // cn:错误
        md += QStringLiteral("## [") + level + QStringLiteral("] ") + e.ruleText + QStringLiteral("\n\n");
        if (e.columnMissing) {
            md += tr("Column missing (rule skipped)") + QStringLiteral("\n\n");  // cn:列不存在（规则被跳过）
            continue;
        }
        md += tr("%1 violation(s)").arg(e.rows.size()) + QStringLiteral("\n\n");  // cn:%1 处违规
        if (!e.rowTexts.isEmpty()) {
            for (const QString& rt : std::as_const(e.rowTexts)) {
                md += QStringLiteral("- ") + rt + QStringLiteral("\n");
            }
            md += QStringLiteral("\n");
        }
    }
    return md;
}

/**
 * @brief 导出（现成 DAMarkdownExporter：HTML/PDF/Word，§4.3-5，不新增导出框架）
 */
void DADialogValidationReport::onExport()
{
    const QString path = QFileDialog::getSaveFileName(this,
                                                      tr("Export Validation Report"),  // cn:导出校验报告
                                                      QString(),
                                                      tr("HTML (*.html);;PDF (*.pdf);;Word (*.docx)"));
    if (path.isEmpty()) {
        return;
    }
    DAMarkdownExporter::Format fmt = DAMarkdownExporter::Format::Html;
    const QString suffix           = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("pdf")) {
        fmt = DAMarkdownExporter::Format::Pdf;
    } else if (suffix == QLatin1String("docx")) {
        fmt = DAMarkdownExporter::Format::Docx;
    } else if (!path.contains(QLatin1Char('.'))) {
        // 无后缀默认补 .html（避免 DAMarkdownExporter 因未知后缀失败）
        const QString withSuffix = path + QStringLiteral(".html");
        DAMarkdownExporter exporter(this);
        QString err;
        if (!exporter.exportToFile(buildMarkdown(), withSuffix, fmt, QString(), &err)) {
            mLabelSummary->setText(tr("Export failed: %1").arg(err));  // cn:导出失败：%1
        }
        return;
    }
    DAMarkdownExporter exporter(this);
    QString err;
    if (!exporter.exportToFile(buildMarkdown(), path, fmt, QString(), &err)) {
        mLabelSummary->setText(tr("Export failed: %1").arg(err));  // cn:导出失败：%1
    }
}

/**
 * @brief 清除违规高亮（只清 Source::Validation，来源隔离，红线2）
 */
void DADialogValidationReport::onClearHighlights()
{
    Q_EMIT clearHighlightsRequested();
}

}  // end of namespace DA
