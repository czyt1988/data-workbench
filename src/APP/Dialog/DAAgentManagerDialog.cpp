#include "DAAgentManagerDialog.h"
#include "DAAgentEditorDialog.h"
#include "DAAgentPromptOps.h"
#include "DAAgentPrompt.h"
// Qt
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>

namespace DA
{
DAAgentManagerDialog::DAAgentManagerDialog(DAAgentPromptOps* ops, QWidget* parent)
    : QDialog(parent), m_ops(ops)
{
    buildUi();
    refreshList();
    setWindowTitle(tr("Agent Manager"));  //cn:Agent 管理
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void DAAgentManagerDialog::buildUi()
{
    resize(520, 400);
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    auto* topLayout = new QHBoxLayout();
    topLayout->setSpacing(8);

    // 左侧 agent 列表
    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    topLayout->addWidget(m_list, 1);

    // 右侧按钮区
    auto* btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(8);
    m_addBtn = new QPushButton(QIcon(":/da/icon/agent-add.svg"), tr("Add"), this);      //cn:添加
    m_editBtn = new QPushButton(QIcon(":/da/icon/agent-edit.svg"), tr("Edit"), this);    //cn:修改
    m_deleteBtn = new QPushButton(QIcon(":/da/icon/agent-delete.svg"), tr("Delete"), this);  //cn:删除
    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_editBtn);
    btnLayout->addWidget(m_deleteBtn);
    btnLayout->addStretch(1);
    topLayout->addLayout(btnLayout);
    mainLayout->addLayout(topLayout, 1);

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
} // namespace DA
