// DADialogAgentSessionManager.cpp
#include "DADialogAgentSessionManager.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFont>

namespace DA
{

// 列索引
enum SessionColumn
{
    ColTitle    = 0,
    ColMessages = 1,
    ColUpdated  = 2,
    ColumnCount
};

// item data role
static constexpr int RoleSessionId = Qt::UserRole;        // 会话 ID
static constexpr int RoleRawTitle  = Qt::UserRole + 1;    // 原始标题（未做「(untitled)」替换）

DADialogAgentSessionManager::DADialogAgentSessionManager(const QVariantList& sessions,
                                                         const QString& currentSessionId,
                                                         QWidget* parent)
    : QDialog(parent)
    , m_table(nullptr)
    , m_switchBtn(nullptr)
    , m_renameBtn(nullptr)
    , m_deleteBtn(nullptr)
    , m_closeBtn(nullptr)
    , m_currentSessionId(currentSessionId)
{
    setWindowTitle(tr("Session Manager"));  // cn:会话管理
    setMinimumSize(520, 360);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    QLabel* hint = new QLabel(tr("Double-click a session to switch:"), this);  // cn:双击切换会话：
    mainLayout->addWidget(hint);

    // ---- 会话表格 ----
    m_table = new QTableWidget(this);
    m_table->setColumnCount(ColumnCount);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setHorizontalHeaderLabels(QStringList()
                                       << tr("Title")     // cn:标题
                                       << tr("Messages")  // cn:消息数
                                       << tr("Updated"));  // cn:更新时间
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(ColTitle, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColMessages, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColUpdated, QHeaderView::ResizeToContents);
    mainLayout->addWidget(m_table, 1);

    // ---- 底部按钮行（右对齐） ----
    QWidget* btnBar = new QWidget(this);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnBar);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(6);
    btnLayout->addStretch(1);
    m_switchBtn = new QPushButton(tr("Switch"), btnBar);  // cn:切换
    m_switchBtn->setDefault(true);
    m_renameBtn = new QPushButton(tr("Rename"), btnBar);  // cn:重命名
    m_deleteBtn = new QPushButton(tr("Delete"), btnBar);  // cn:删除
    m_closeBtn  = new QPushButton(tr("Close"), btnBar);   // cn:关闭
    btnLayout->addWidget(m_switchBtn);
    btnLayout->addWidget(m_renameBtn);
    btnLayout->addWidget(m_deleteBtn);
    btnLayout->addWidget(m_closeBtn);
    mainLayout->addWidget(btnBar);

    // ---- 数据填充 ----
    populateSessions(sessions);

    // ---- 信号连接 ----
    connect(m_switchBtn, &QPushButton::clicked, this, &DADialogAgentSessionManager::onSwitchClicked);
    connect(m_renameBtn, &QPushButton::clicked, this, &DADialogAgentSessionManager::onRenameClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &DADialogAgentSessionManager::onDeleteClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    // 双击行 = 切换
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &DADialogAgentSessionManager::onItemDoubleClicked);
    // 选中变化刷新按钮可用态
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &DADialogAgentSessionManager::onSelectionChanged);

    updateButtonStates();
}

void DADialogAgentSessionManager::populateSessions(const QVariantList& sessions)
{
    m_table->setRowCount(0);  // 清空
    m_table->setRowCount(sessions.size());
    int selectRow = -1;
    for (int i = 0; i < sessions.size(); ++i) {
        QVariantMap vm = sessions.at(i).toMap();
        QString id    = vm.value("id").toString();
        QString title = vm.value("title").toString();
        int msgCount  = vm.value("messageCount", 0).toInt();
        QString updated = formatTimestamp(vm.value("updatedAt").toString());

        // Title：空标题显示「(untitled)」
        QString displayTitle = title.isEmpty() ? tr("(untitled)")  // cn:（未命名）
                                                : title;

        auto* titleItem = new QTableWidgetItem(displayTitle);
        titleItem->setData(RoleSessionId, id);
        titleItem->setData(RoleRawTitle, title);

        auto* msgItem = new QTableWidgetItem(QString::number(msgCount));
        msgItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto* updatedItem = new QTableWidgetItem(updated);
        updatedItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // 当前会话行加粗
        if (id == m_currentSessionId) {
            QFont bold = titleItem->font();
            bold.setBold(true);
            titleItem->setFont(bold);
            msgItem->setFont(bold);
            updatedItem->setFont(bold);
            selectRow = i;
        }

        m_table->setItem(i, ColTitle, titleItem);
        m_table->setItem(i, ColMessages, msgItem);
        m_table->setItem(i, ColUpdated, updatedItem);
    }

    // 选中当前会话行（屏蔽信号避免触发 onSelectionChanged 的副作用）
    if (selectRow >= 0) {
        m_table->blockSignals(true);
        m_table->selectRow(selectRow);
        m_table->blockSignals(false);
    }
    updateButtonStates();
}

QString DADialogAgentSessionManager::formatTimestamp(const QString& iso)
{
    if (iso.isEmpty()) return QString();
    QDateTime dt = QDateTime::fromString(iso, Qt::ISODateWithMs);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(iso, Qt::ISODate);
    }
    if (!dt.isValid()) {
        return iso;  // 解析失败，回退原始串
    }
    return dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
}

int DADialogAgentSessionManager::currentSelectedRow() const
{
    QList<QTableWidgetItem*> sel = m_table->selectedItems();
    if (sel.isEmpty()) return -1;
    return sel.first()->row();
}

QString DADialogAgentSessionManager::sessionIdAt(int row) const
{
    if (row < 0 || row >= m_table->rowCount()) return QString();
    if (auto* it = m_table->item(row, ColTitle)) {
        return it->data(RoleSessionId).toString();
    }
    return QString();
}

void DADialogAgentSessionManager::updateButtonStates()
{
    bool hasSelection = (currentSelectedRow() >= 0);
    m_switchBtn->setEnabled(hasSelection);
    m_renameBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
}

void DADialogAgentSessionManager::onSwitchClicked()
{
    int row = currentSelectedRow();
    if (row < 0) return;
    QString sid = sessionIdAt(row);
    if (sid.isEmpty()) return;
    emit switchRequested(sid);
    accept();  // 关闭对话框，便于查看聊天切换效果
}

void DADialogAgentSessionManager::onItemDoubleClicked(int row)
{
    Q_UNUSED(row);
    onSwitchClicked();
}

void DADialogAgentSessionManager::onRenameClicked()
{
    int row = currentSelectedRow();
    if (row < 0) return;
    if (auto* titleItem = m_table->item(row, ColTitle)) {
        QString sid = titleItem->data(RoleSessionId).toString();
        QString oldTitle = titleItem->data(RoleRawTitle).toString();  // 原始标题（空=未命名）
        bool ok = false;
        QString newTitle = QInputDialog::getText(this,
            tr("Rename Session"),  // cn:重命名会话
            tr("New title:"),      // cn:新标题：
            QLineEdit::Normal,
            oldTitle,
            &ok);
        if (!ok || newTitle.trimmed().isEmpty()) return;
        newTitle = newTitle.trimmed();
        // 本地乐观更新该行
        titleItem->setData(RoleRawTitle, newTitle);
        titleItem->setText(newTitle.isEmpty() ? tr("(untitled)") : newTitle);
        emit renameRequested(sid, newTitle);
    }
}

void DADialogAgentSessionManager::onDeleteClicked()
{
    int row = currentSelectedRow();
    if (row < 0) return;
    if (auto* titleItem = m_table->item(row, ColTitle)) {
        QString sid = titleItem->data(RoleSessionId).toString();
        QString displayTitle = titleItem->text();
        int ret = QMessageBox::question(this,
            tr("Delete Session"),  // cn:删除会话
            tr("Delete session \"%1\"? This cannot be undone.").arg(displayTitle),  // cn:删除会话「%1」？此操作不可撤销。
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
        // 本地移除该行
        m_table->removeRow(row);
        // 若删除的是当前会话行，清空选中
        if (sid == m_currentSessionId) {
            m_table->clearSelection();
        }
        emit deleteRequested(sid);
        updateButtonStates();
    }
}

void DADialogAgentSessionManager::onSelectionChanged()
{
    updateButtonStates();
}

}  // namespace DA
