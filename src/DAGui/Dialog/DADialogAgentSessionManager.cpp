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

/**
 * @brief 构造函数
 * @param sessions 会话列表 payload（每元素 QVariantMap{id,title,updatedAt,messageCount}）
 * @param currentSessionId 当前活跃会话 ID（用于高亮与默认选中）
 * @param parent 父窗口
 */
DADialogAgentSessionManager::DADialogAgentSessionManager(const QVariantList& sessions,
                                                         const QString& currentSessionId,
                                                         QWidget* parent)
    : QDialog(parent)
    , mTable(nullptr)
    , mSwitchBtn(nullptr)
    , mRenameBtn(nullptr)
    , mDeleteBtn(nullptr)
    , mCloseBtn(nullptr)
    , mCurrentSessionId(currentSessionId)
{
    setWindowTitle(tr("Session Manager"));  // cn:会话管理
    setMinimumSize(520, 360);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    QLabel* hint = new QLabel(tr("Double-click a session to switch:"), this);  // cn:双击切换会话：
    mainLayout->addWidget(hint);

    // ---- 会话表格 ----
    mTable = new QTableWidget(this);
    mTable->setColumnCount(ColumnCount);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTable->setHorizontalHeaderLabels(QStringList()
                                       << tr("Title")     // cn:标题
                                       << tr("Messages")  // cn:消息数
                                       << tr("Updated"));  // cn:更新时间
    mTable->verticalHeader()->setVisible(false);
    mTable->horizontalHeader()->setStretchLastSection(false);
    mTable->horizontalHeader()->setSectionResizeMode(ColTitle, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(ColMessages, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(ColUpdated, QHeaderView::ResizeToContents);
    mainLayout->addWidget(mTable, 1);

    // ---- 底部按钮行（右对齐） ----
    QWidget* btnBar = new QWidget(this);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnBar);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(6);
    btnLayout->addStretch(1);
    mSwitchBtn = new QPushButton(tr("Switch"), btnBar);  // cn:切换
    mSwitchBtn->setDefault(true);
    mRenameBtn = new QPushButton(tr("Rename"), btnBar);  // cn:重命名
    mDeleteBtn = new QPushButton(tr("Delete"), btnBar);  // cn:删除
    mCloseBtn  = new QPushButton(tr("Close"), btnBar);   // cn:关闭
    btnLayout->addWidget(mSwitchBtn);
    btnLayout->addWidget(mRenameBtn);
    btnLayout->addWidget(mDeleteBtn);
    btnLayout->addWidget(mCloseBtn);
    mainLayout->addWidget(btnBar);

    // ---- 数据填充 ----
    populateSessions(sessions);

    // ---- 信号连接 ----
    connect(mSwitchBtn, &QPushButton::clicked, this, &DADialogAgentSessionManager::onSwitchClicked);
    connect(mRenameBtn, &QPushButton::clicked, this, &DADialogAgentSessionManager::onRenameClicked);
    connect(mDeleteBtn, &QPushButton::clicked, this, &DADialogAgentSessionManager::onDeleteClicked);
    connect(mCloseBtn, &QPushButton::clicked, this, &QDialog::reject);
    // 双击行 = 切换
    connect(mTable, &QTableWidget::cellDoubleClicked, this, &DADialogAgentSessionManager::onItemDoubleClicked);
    // 选中变化刷新按钮可用态
    connect(mTable, &QTableWidget::itemSelectionChanged, this, &DADialogAgentSessionManager::onSelectionChanged);

    updateButtonStates();
}

/**
 * @brief 填充会话表格
 * @param sessions 会话列表 payload
 */
void DADialogAgentSessionManager::populateSessions(const QVariantList& sessions)
{
    mTable->setRowCount(0);  // 清空
    mTable->setRowCount(sessions.size());
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
        if (id == mCurrentSessionId) {
            QFont bold = titleItem->font();
            bold.setBold(true);
            titleItem->setFont(bold);
            msgItem->setFont(bold);
            updatedItem->setFont(bold);
            selectRow = i;
        }

        mTable->setItem(i, ColTitle, titleItem);
        mTable->setItem(i, ColMessages, msgItem);
        mTable->setItem(i, ColUpdated, updatedItem);
    }

    // 选中当前会话行（屏蔽信号避免触发 onSelectionChanged 的副作用）
    if (selectRow >= 0) {
        mTable->blockSignals(true);
        mTable->selectRow(selectRow);
        mTable->blockSignals(false);
    }
    updateButtonStates();
}

/**
 * @brief 把 ISO8601 时间串格式化为本地显示串
 * @param iso ISO8601 时间串
 * @return 格式化后的本地时间串（yyyy-MM-dd HH:mm），解析失败时回退原始串
 */
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

/**
 * @brief 获取当前选中行
 * @return 当前选中行号，无选中返回 -1
 */
int DADialogAgentSessionManager::currentSelectedRow() const
{
    QList<QTableWidgetItem*> sel = mTable->selectedItems();
    if (sel.isEmpty()) return -1;
    return sel.first()->row();
}

/**
 * @brief 取指定行的会话 ID
 * @param row 行号
 * @return 会话 ID，无选中返回空串
 */
QString DADialogAgentSessionManager::sessionIdAt(int row) const
{
    if (row < 0 || row >= mTable->rowCount()) return QString();
    if (auto* it = mTable->item(row, ColTitle)) {
        return it->data(RoleSessionId).toString();
    }
    return QString();
}

/**
 * @brief 根据当前选中刷新按钮可用态
 */
void DADialogAgentSessionManager::updateButtonStates()
{
    bool hasSelection = (currentSelectedRow() >= 0);
    mSwitchBtn->setEnabled(hasSelection);
    mRenameBtn->setEnabled(hasSelection);
    mDeleteBtn->setEnabled(hasSelection);
}

/**
 * @brief 切换按钮点击槽函数
 */
void DADialogAgentSessionManager::onSwitchClicked()
{
    int row = currentSelectedRow();
    if (row < 0) return;
    QString sid = sessionIdAt(row);
    if (sid.isEmpty()) return;
    emit switchRequested(sid);
    accept();  // 关闭对话框，便于查看聊天切换效果
}

/**
 * @brief 表格项双击槽函数，触发切换
 * @param row 双击的行号
 */
void DADialogAgentSessionManager::onItemDoubleClicked(int row)
{
    Q_UNUSED(row);
    onSwitchClicked();
}

/**
 * @brief 重命名按钮点击槽函数
 */
void DADialogAgentSessionManager::onRenameClicked()
{
    int row = currentSelectedRow();
    if (row < 0) return;
    if (auto* titleItem = mTable->item(row, ColTitle)) {
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

/**
 * @brief 删除按钮点击槽函数
 */
void DADialogAgentSessionManager::onDeleteClicked()
{
    int row = currentSelectedRow();
    if (row < 0) return;
    if (auto* titleItem = mTable->item(row, ColTitle)) {
        QString sid = titleItem->data(RoleSessionId).toString();
        QString displayTitle = titleItem->text();
        int ret = QMessageBox::question(this,
            tr("Delete Session"),  // cn:删除会话
            tr("Delete session \"%1\"? This cannot be undone.").arg(displayTitle),  // cn:删除会话「%1」？此操作不可撤销。
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
        // 本地移除该行
        mTable->removeRow(row);
        // 若删除的是当前会话行，清空选中
        if (sid == mCurrentSessionId) {
            mTable->clearSelection();
        }
        emit deleteRequested(sid);
        updateButtonStates();
    }
}

/**
 * @brief 选中变化槽函数
 */
void DADialogAgentSessionManager::onSelectionChanged()
{
    updateButtonStates();
}

}  // namespace DA
