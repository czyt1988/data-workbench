#include "DAAgentSubagentEditDialog.h"
#include "DAMarkdownHighlighter.h"
// Qt
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QStyle>
#include <QMessageBox>
#include <QSplitter>
#include <QFont>

namespace DA
{
/**
 * @brief 构造：预填编辑态（def 非空时 name 只读），新增态表单全空
 */
DAAgentSubagentEditDialog::DAAgentSubagentEditDialog(const QJsonObject& def,
                                                     const QJsonArray& existingDefs,
                                                     const QStringList& registeredTools,
                                                     QWidget* parent)
    : QDialog(parent), m_registeredTools(registeredTools)
{
    for (const QJsonValue& v : existingDefs) {
        m_existingNames << v.toObject().value("name").toString();
    }
    buildUi();

    const bool isEdit = !def.isEmpty();
    if (isEdit) {
        m_nameEdit->setText(def.value("name").toString());
        m_descEdit->setText(def.value("description").toString());
        m_promptEdit->setPlainText(def.value("system_prompt").toString());
        // 编辑态 name 只读：重命名 = 新建 + 删除，由 manager 的 saveSubagent(oldName) 承担
        m_nameEdit->setReadOnly(true);
    }
    // 工具白名单勾选（定义中的 tools ∪ 已注册工具为全集；
    // 引用未注册工具时保留为禁用勾选项，防静默丢失）
    const QStringList defTools = [def]() {
        QStringList l;
        for (const QJsonValue& v : def.value("tools").toArray()) {
            l << v.toString();
        }
        return l;
    }();
    for (const QString& t : m_registeredTools) {
        QListWidgetItem* item = new QListWidgetItem(t, m_toolsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(defTools.contains(t) ? Qt::Checked : Qt::Unchecked);
    }
    for (const QString& t : defTools) {
        if (!m_registeredTools.contains(t)) {
            QListWidgetItem* item = new QListWidgetItem(t, m_toolsList);
            item->setFlags((item->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEnabled);
            item->setCheckState(Qt::Checked);
            item->setToolTip(tr("Unregistered tool (kept in definition)"));  //cn:未注册的工具（保留在定义中）
        }
    }
    setWindowTitle(isEdit ? tr("Edit Subagent")   //cn:编辑子 Agent
                          : tr("New Subagent"));  //cn:新增子 Agent
}

void DAAgentSubagentEditDialog::buildUi()
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // ---- 顶部结构化字段 ----
    auto* form = new QFormLayout();
    form->setSpacing(6);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("snake_case name used by dispatch"));  //cn:snake_case 名称（派发时引用）
    form->addRow(tr("Name:"), m_nameEdit);  //cn:名称：
    m_descEdit = new QLineEdit(this);
    m_descEdit->setPlaceholderText(tr("Short description shown to the main agent"));  //cn:展示给主 Agent 的简短描述
    form->addRow(tr("Description:"), m_descEdit);  //cn:描述：
    mainLayout->addLayout(form);

    // ---- 中部：工具白名单 + 正文（左右分割） ----
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // 工具白名单（复选框列表）
    auto* toolsPanel = new QWidget(splitter);
    auto* toolsLay = new QVBoxLayout(toolsPanel);
    toolsLay->setContentsMargins(0, 0, 0, 0);
    toolsLay->setSpacing(4);
    auto* toolsLabel = new QLabel(tr("Allowed tools:"), toolsPanel);  //cn:工具白名单：
    toolsLabel->setToolTip(tr("Only checked tools can be called by this subagent. "
                              "Permission rules still apply on every call."));  //cn:子 Agent 只能调用勾选的工具；每次调用仍受权限规则约束
    toolsLay->addWidget(toolsLabel);
    m_toolsList = new QListWidget(toolsPanel);
    toolsLay->addWidget(m_toolsList, 1);
    splitter->addWidget(toolsPanel);

    // 正文（Markdown 系统提示词 + 高亮，复用 DAAgentEditorDialog 同款）
    auto* promptPanel = new QWidget(splitter);
    auto* promptLay = new QVBoxLayout(promptPanel);
    promptLay->setContentsMargins(0, 0, 0, 0);
    promptLay->setSpacing(4);
    promptLay->addWidget(new QLabel(tr("System prompt:"), promptPanel));  //cn:系统提示词：
    m_promptEdit = new QPlainTextEdit(promptPanel);
    QFont monoFont("Consolas");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(10);
    m_promptEdit->setFont(monoFont);
    m_promptEdit->setPlaceholderText(tr("Write the subagent instructions here (Markdown supported). "
                                        "It should finish autonomously and end with a structured summary."));  //cn:在此编写子 Agent 指引（支持 Markdown），应自主完成任务并输出结构化总结
    m_highlighter = new DAMarkdownHighlighter(m_promptEdit->document());
    promptLay->addWidget(m_promptEdit, 1);
    splitter->addWidget(promptPanel);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({ 220, 420 });
    mainLayout->addWidget(splitter, 1);

    // ---- 底部按钮 ----
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    auto* saveBtn = new QPushButton(tr("Save"), this);    //cn:保存
    saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);  //cn:取消
    cancelBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    resize(760, 520);

    connect(saveBtn, &QPushButton::clicked, this, &DAAgentSubagentEditDialog::onSaveClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

/**
 * @brief 保存前校验：name 非空 snake_case 且不与既有定义重名（编辑态 name 只读自然豁免）
 */
bool DAAgentSubagentEditDialog::validate()
{
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Tip"),                    //cn:提示
                             tr("Name cannot be empty"));  //cn:名称不能为空
        m_nameEdit->setFocus();
        return false;
    }
    if (m_existingNames.contains(name) && !m_nameEdit->isReadOnly()) {
        QMessageBox::warning(this, tr("Tip"),  //cn:提示
                             tr("A subagent named \"%1\" already exists, please choose another name").arg(name));  //cn:已存在同名子 Agent「%1」，请更换名称
        m_nameEdit->setFocus();
        return false;
    }
    return true;
}

void DAAgentSubagentEditDialog::onSaveClicked()
{
    if (!validate()) {
        return;
    }
    accept();
}

/**
 * @brief 收集表单为定义 JSON（{name,description,tools,system_prompt}）
 *
 * tools 只收集勾选项（未注册的禁用项勾选态也会被收集，Python 侧求交剔除兜底）
 */
QJsonObject DAAgentSubagentEditDialog::getDefinition() const
{
    QJsonObject def;
    def["name"]           = m_nameEdit->text().trimmed();
    def["description"]    = m_descEdit->text().trimmed();
    QJsonArray tools;
    for (int i = 0; i < m_toolsList->count(); ++i) {
        QListWidgetItem* item = m_toolsList->item(i);
        if (item->checkState() == Qt::Checked) {
            tools.append(item->text());
        }
    }
    def["tools"]          = tools;
    def["system_prompt"]  = m_promptEdit->toPlainText();
    return def;
}

QString DAAgentSubagentEditDialog::getName() const
{
    return m_nameEdit->text().trimmed();
}
} // namespace DA
