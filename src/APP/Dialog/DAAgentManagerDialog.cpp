#include "DAAgentManagerDialog.h"
#include "DAAgentEditorDialog.h"
#include "DAAgentSubagentEditDialog.h"
#include "DAAgentPromptOps.h"
#include "DAAgentPrompt.h"
#include "DAAgentInterface.h"
// Qt
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>

namespace DA
{
DAAgentManagerDialog::DAAgentManagerDialog(DAAgentPromptOps* ops,
                                           DAAgentInterface* agent,
                                           QWidget* parent)
    : QDialog(parent), m_ops(ops), m_agent(agent)
{
    buildUi();
    refreshList();
    refreshSubagentList();
    setWindowTitle(tr("Agent Manager"));  //cn:Agent 管理
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void DAAgentManagerDialog::buildUi()
{
    resize(560, 440);
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    auto* tabs = new QTabWidget(this);
    mainLayout->addWidget(tabs, 1);

    // ---- Tab1：提示词库（原有内容平移） ----
    auto* promptPage = new QWidget(this);
    auto* topLayout = new QHBoxLayout(promptPage);
    topLayout->setContentsMargins(0, 8, 0, 0);
    topLayout->setSpacing(8);

    // 左侧 agent 列表
    m_list = new QListWidget(promptPage);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    topLayout->addWidget(m_list, 1);

    // 右侧按钮区
    auto* btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(8);
    m_addBtn = new QPushButton(QIcon(":/da/icon/agent-add.svg"), tr("Add"), promptPage);      //cn:添加
    m_editBtn = new QPushButton(QIcon(":/da/icon/agent-edit.svg"), tr("Edit"), promptPage);    //cn:修改
    m_deleteBtn = new QPushButton(QIcon(":/da/icon/agent-delete.svg"), tr("Delete"), promptPage);  //cn:删除
    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_editBtn);
    btnLayout->addWidget(m_deleteBtn);
    btnLayout->addStretch(1);
    topLayout->addLayout(btnLayout);
    tabs->addTab(promptPage, tr("Prompt Library"));  //cn:提示词库

    // ---- Tab2：子 Agent ----
    buildSubagentTab(tabs);

    // 底部关闭
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch(1);
    auto* closeBtn = new QPushButton(tr("Close"), this);  //cn:关闭
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);

    connect(m_addBtn, &QPushButton::clicked, this, &DAAgentManagerDialog::onAddClicked);
    connect(m_editBtn, &QPushButton::clicked, this, &DAAgentManagerDialog::onEditClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &DAAgentManagerDialog::onDeleteClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &DAAgentManagerDialog::onItemDoubleClicked);
    connect(m_list, &QListWidget::itemSelectionChanged, this, &DAAgentManagerDialog::onSelectionChanged);
}

void DAAgentManagerDialog::buildSubagentTab(QTabWidget* tabs)
{
    auto* page = new QWidget(this);
    auto* topLayout = new QHBoxLayout(page);
    topLayout->setContentsMargins(0, 8, 0, 0);
    topLayout->setSpacing(8);

    // 左侧子 agent 定义列表（name + description 摘要）
    m_subagentList = new QListWidget(page);
    m_subagentList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_subagentList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_subagentList->setToolTip(tr("Subagent definitions available for dispatch. "
                                  "Edit to change tools or instructions."));  //cn:可派发的子 Agent 定义，编辑可调整工具白名单与提示词
    topLayout->addWidget(m_subagentList, 1);

    // 右侧按钮区
    auto* btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(8);
    m_subagentAddBtn = new QPushButton(QIcon(":/da/icon/agent-add.svg"), tr("Add"), page);      //cn:添加
    m_subagentEditBtn = new QPushButton(QIcon(":/da/icon/agent-edit.svg"), tr("Edit"), page);    //cn:修改
    m_subagentDeleteBtn = new QPushButton(QIcon(":/da/icon/agent-delete.svg"), tr("Delete"), page);  //cn:删除
    btnLayout->addWidget(m_subagentAddBtn);
    btnLayout->addWidget(m_subagentEditBtn);
    btnLayout->addWidget(m_subagentDeleteBtn);
    btnLayout->addStretch(1);
    topLayout->addLayout(btnLayout);
    tabs->addTab(page, tr("Subagents"));  //cn:子 Agent

    connect(m_subagentAddBtn, &QPushButton::clicked, this, &DAAgentManagerDialog::onSubagentAddClicked);
    connect(m_subagentEditBtn, &QPushButton::clicked, this, &DAAgentManagerDialog::onSubagentEditClicked);
    connect(m_subagentDeleteBtn, &QPushButton::clicked, this, &DAAgentManagerDialog::onSubagentDeleteClicked);
    connect(m_subagentList, &QListWidget::itemDoubleClicked, this, &DAAgentManagerDialog::onSubagentItemDoubleClicked);
    connect(m_subagentList, &QListWidget::itemSelectionChanged, this, &DAAgentManagerDialog::onSubagentSelectionChanged);
}

void DAAgentManagerDialog::refreshList()
{
    m_list->clear();
    if (!m_ops) {
        return;
    }
    for (const DAAgentPrompt& a : m_ops->agentPrompts()) {
        auto* item = new QListWidgetItem(a.title, m_list);
        item->setData(Qt::UserRole, a.title);
        m_list->addItem(item);
    }
    updateButtonStates();
}

void DAAgentManagerDialog::updateButtonStates()
{
    bool hasSelection = (currentSelectedTitle() != QString());
    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
}

QString DAAgentManagerDialog::currentSelectedTitle() const
{
    int row = m_list->currentRow();
    if (row < 0) {
        return QString();
    }
    QListWidgetItem* item = m_list->item(row);
    return item ? item->data(Qt::UserRole).toString() : QString();
}

QStringList DAAgentManagerDialog::existingTitles() const
{
    QStringList titles;
    if (m_ops) {
        for (const DAAgentPrompt& a : m_ops->agentPrompts()) {
            titles << a.title;
        }
    }
    return titles;
}

void DAAgentManagerDialog::onAddClicked()
{
    DAAgentEditorDialog dlg(QString(), QString(), existingTitles(), QString(), this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    QString title = dlg.getTitle();
    QString content = dlg.getContent();
    if (m_ops->saveAgent(title, content)) {
        refreshList();
        // 选中新加项
        for (int i = 0; i < m_list->count(); ++i) {
            if (m_list->item(i)->data(Qt::UserRole).toString() == title) {
                m_list->setCurrentRow(i);
                break;
            }
        }
    } else {
        QMessageBox::warning(this,
                             tr("Tip"),          //cn:提示
                             tr("Save failed")); //cn:保存失败
    }
}

void DAAgentManagerDialog::onEditClicked()
{
    QString title = currentSelectedTitle();
    if (title.isEmpty()) {
        return;
    }
    // 从接口拿当前内容
    DAAgentPrompt current;
    if (m_ops) {
        for (const DAAgentPrompt& a : m_ops->agentPrompts()) {
            if (a.title == title) {
                current = a;
                break;
            }
        }
    }
    if (!current.isValid()) {
        return;
    }
    DAAgentEditorDialog dlg(current.title, current.content, existingTitles(), current.title, this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    QString newTitle = dlg.getTitle();
    QString newContent = dlg.getContent();
    if (m_ops->saveAgent(newTitle, newContent, title)) {
        refreshList();
        for (int i = 0; i < m_list->count(); ++i) {
            if (m_list->item(i)->data(Qt::UserRole).toString() == newTitle) {
                m_list->setCurrentRow(i);
                break;
            }
        }
    } else {
        QMessageBox::warning(this,
                             tr("Tip"),          //cn:提示
                             tr("Save failed")); //cn:保存失败
    }
}

void DAAgentManagerDialog::onItemDoubleClicked(QListWidgetItem*)
{
    onEditClicked();
}

void DAAgentManagerDialog::onDeleteClicked()
{
    QString title = currentSelectedTitle();
    if (title.isEmpty()) {
        return;
    }
    int ret = QMessageBox::question(this,
                                      tr("Confirm Delete"),  //cn:确认删除
                                      tr("Are you sure to delete agent \"%1\"?").arg(title),  //cn:确定删除 Agent「%1」吗？
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }
    if (m_ops->deleteAgent(title)) {
        refreshList();
    } else {
        QMessageBox::warning(this,
                             tr("Tip"),            //cn:提示
                             tr("Delete failed")); //cn:删除失败
    }
}

void DAAgentManagerDialog::onSelectionChanged()
{
    updateButtonStates();
}

// ---- Tab2：子 Agent ----

/**
 * @brief 刷新子 agent 定义列表（数据源 subagentDefinitions()）
 *
 * 显示 name（粗体）+ description 单行摘要，UserRole 存 name 供选中提取
 */
void DAAgentManagerDialog::refreshSubagentList()
{
    m_subagentList->clear();
    if (!m_agent) {
        updateSubagentButtonStates();
        return;
    }
    const QJsonArray defs = m_agent->subagentDefinitions();
    for (const QJsonValue& v : defs) {
        const QJsonObject obj = v.toObject();
        const QString name = obj.value("name").toString();
        if (name.isEmpty()) {
            continue;
        }
        QString desc = obj.value("description").toString();
        if (desc.size() > 80) {
            desc = desc.left(80) + QStringLiteral("...");
        }
        auto* item = new QListWidgetItem(name + (desc.isEmpty() ? QString() : QStringLiteral("  —  ") + desc));
        item->setData(Qt::UserRole, name);
        m_subagentList->addItem(item);
    }
    updateSubagentButtonStates();
}

void DAAgentManagerDialog::updateSubagentButtonStates()
{
    const bool hasSelection = (currentSelectedSubagent() != QString());
    m_subagentEditBtn->setEnabled(hasSelection);
    m_subagentDeleteBtn->setEnabled(hasSelection);
}

QString DAAgentManagerDialog::currentSelectedSubagent() const
{
    QListWidgetItem* item = m_subagentList->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

/**
 * @brief 新增子 agent：弹结构化编辑器（空表单），保存调 saveSubagent
 */
void DAAgentManagerDialog::onSubagentAddClicked()
{
    if (!m_agent) {
        return;
    }
    DAAgentSubagentEditDialog dlg(QJsonObject(),
                                  m_agent->subagentDefinitions(),
                                  m_agent->registeredToolNames(),
                                  this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const QString name = dlg.getName();
    if (m_agent->saveSubagent(dlg.getDefinition())) {
        refreshSubagentList();
        for (int i = 0; i < m_subagentList->count(); ++i) {
            if (m_subagentList->item(i)->data(Qt::UserRole).toString() == name) {
                m_subagentList->setCurrentRow(i);
                break;
            }
        }
    } else {
        QMessageBox::warning(this,
                             tr("Tip"),          //cn:提示
                             tr("Save failed")); //cn:保存失败
    }
}

/**
 * @brief 编辑选中子 agent：弹结构化编辑器（预填，name 只读），保存调 saveSubagent(oldName)
 */
void DAAgentManagerDialog::onSubagentEditClicked()
{
    if (!m_agent) {
        return;
    }
    const QString name = currentSelectedSubagent();
    if (name.isEmpty()) {
        return;
    }
    QJsonObject current;
    for (const QJsonValue& v : m_agent->subagentDefinitions()) {
        const QJsonObject obj = v.toObject();
        if (obj.value("name").toString() == name) {
            current = obj;
            break;
        }
    }
    if (current.isEmpty()) {
        return;
    }
    DAAgentSubagentEditDialog dlg(current,
                                  m_agent->subagentDefinitions(),
                                  m_agent->registeredToolNames(),
                                  this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    if (m_agent->saveSubagent(dlg.getDefinition(), name)) {
        refreshSubagentList();
        for (int i = 0; i < m_subagentList->count(); ++i) {
            if (m_subagentList->item(i)->data(Qt::UserRole).toString() == name) {
                m_subagentList->setCurrentRow(i);
                break;
            }
        }
    } else {
        QMessageBox::warning(this,
                             tr("Tip"),          //cn:提示
                             tr("Save failed")); //cn:保存失败
    }
}

/**
 * @brief 删除选中子 agent：确认后调 deleteSubagent
 */
void DAAgentManagerDialog::onSubagentDeleteClicked()
{
    if (!m_agent) {
        return;
    }
    const QString name = currentSelectedSubagent();
    if (name.isEmpty()) {
        return;
    }
    int ret = QMessageBox::question(this,
                                      tr("Confirm Delete"),  //cn:确认删除
                                      tr("Are you sure to delete subagent \"%1\"?").arg(name),  //cn:确定删除子 Agent「%1」吗？
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }
    if (m_agent->deleteSubagent(name)) {
        refreshSubagentList();
    } else {
        QMessageBox::warning(this,
                             tr("Tip"),            //cn:提示
                             tr("Delete failed")); //cn:删除失败
    }
}

void DAAgentManagerDialog::onSubagentItemDoubleClicked(QListWidgetItem*)
{
    onSubagentEditClicked();
}

void DAAgentManagerDialog::onSubagentSelectionChanged()
{
    updateSubagentButtonStates();
}
} // namespace DA
