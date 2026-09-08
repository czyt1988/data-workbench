// DADialogAgentSessionManager.cpp
#include "DADialogAgentSessionManager.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QGroupBox>
#include <QStackedWidget>
#include <QCheckBox>
#include <QDateTime>
#include <QFileInfo>
#include <QFont>
#include <QColor>
#include <QLocale>

namespace DA
{

// 列索引
enum SessionColumn
{
    ColTitle    = 0,
    ColState    = 1,
    ColMessages = 2,
    ColUpdated  = 3,
    ColumnCount
};

// item data role
static constexpr int RoleSessionId = Qt::UserRole;        // 会话 ID
static constexpr int RoleRawTitle  = Qt::UserRole + 1;    // 原始标题（未做「(untitled)」替换）
static constexpr int RolePayload   = Qt::UserRole + 2;    // 整份 payload QVariantMap（详情面板取数）
static constexpr int RoleSortValue = Qt::UserRole + 3;    // 数值排序键（消息数/时间 epoch ms）

/**
 * @brief 数值感知排序项：优先按 RoleSortValue 数值比较，无数值时回退文本比较
 */
class SessionSortItem : public QTableWidgetItem
{
public:
    explicit SessionSortItem(const QString& text) : QTableWidgetItem(text) {}
    bool operator<(const QTableWidgetItem& other) const override
    {
        const QVariant a = data(RoleSortValue);
        const QVariant b = other.data(RoleSortValue);
        if (a.isValid() && b.isValid()) {
            return a.toLongLong() < b.toLongLong();
        }
        return QTableWidgetItem::operator<(other);
    }
};

// token 数格式化为人类可读串（<0 显示破折号，<1000 原值，其余 K/M）
static QString formatTokenCount(qint64 n)
{
    if (n < 0) return QStringLiteral("—");
    if (n < 1000) return QString::number(n);
    if (n < 1000000) {
        return QStringLiteral("%1K").arg(QString::number(static_cast<double>(n) / 1000.0, 'f', 1));
    }
    return QStringLiteral("%1M").arg(QString::number(static_cast<double>(n) / 1000000.0, 'f', 1));
}

// 详情面板 token 文本：人类可读 + 千分位精确值（小数值只显示原值）
static QString formatTokenDetail(qint64 n)
{
    if (n < 0) return QStringLiteral("—");
    const QString human = formatTokenCount(n);
    const QString exact = QLocale().toString(n);
    if (human == exact) return exact;
    return QStringLiteral("%1 (%2)").arg(human, exact);
}

// ISO8601 → epoch 毫秒（排序键；无效值返回 0 排最前）
static qint64 epochMsFromIso(const QString& iso)
{
    QDateTime dt = QDateTime::fromString(iso, Qt::ISODateWithMs);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(iso, Qt::ISODate);
    }
    return dt.isValid() ? dt.toMSecsSinceEpoch() : 0;
}

/**
 * @brief 构造函数
 * @param sessions 会话列表 payload（每元素 QVariantMap{id,title,createdAt,updatedAt,messageCount,state,inputTokens,outputTokens,totalTokens}）
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
    , mDetailStack(nullptr)
    , mDetailTitleLabel(nullptr)
    , mDetailStateValue(nullptr)
    , mDetailMsgValue(nullptr)
    , mDetailInValue(nullptr)
    , mDetailOutValue(nullptr)
    , mDetailTotalValue(nullptr)
    , mDetailCreatedValue(nullptr)
    , mDetailUpdatedValue(nullptr)
    , mSummaryLabel(nullptr)
{
    setWindowTitle(tr("Session Manager"));  // cn:会话管理
    setMinimumSize(760, 420);
    mBaseSessions = sessions;

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    QLabel* hint = new QLabel(tr("Double-click a session to switch:"), this);  // cn:双击切换会话：
    mainLayout->addWidget(hint);

    // "全部工程"复选框（决策点 5 方案 c，审计问题 18）：setForeignSessions
    // 注入跨工程存活会话后启用，勾选合并显示（foreign 行禁止切换）
    mAllProjectsCheck = new QCheckBox(tr("Show sessions from all projects"), this);  // cn:显示全部工程的会话
    mAllProjectsCheck->setEnabled(false);
    connect(mAllProjectsCheck, &QCheckBox::toggled, this, [this](bool on) {
        mShowAllProjects = on;
        populateSessions(combinedSessions());
    });
    mainLayout->addWidget(mAllProjectsCheck);

    // ---- 内容区：左侧表格 + 右侧详情面板 ----
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(8);

    // 左侧：会话表格 + 汇总栏
    QWidget* leftWidget = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    mTable = new QTableWidget(leftWidget);
    mTable->setColumnCount(ColumnCount);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTable->setHorizontalHeaderLabels(QStringList()
                                       << tr("Title")     // cn:标题
                                       << tr("State")     // cn:状态
                                       << tr("Messages")  // cn:消息数
                                       << tr("Updated")); // cn:更新时间
    mTable->verticalHeader()->setVisible(false);
    mTable->horizontalHeader()->setStretchLastSection(false);
    mTable->horizontalHeader()->setSectionResizeMode(ColTitle, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(ColState, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(ColMessages, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(ColUpdated, QHeaderView::ResizeToContents);
    mTable->setContextMenuPolicy(Qt::CustomContextMenu);
    leftLayout->addWidget(mTable, 1);

    // 汇总栏：会话总数 + 累计 token
    mSummaryLabel = new QLabel(leftWidget);
    leftLayout->addWidget(mSummaryLabel);

    contentLayout->addWidget(leftWidget, 1);

    // 右侧：详情面板（占位页 / 详情页两页切换）
    QGroupBox* detailGroup = new QGroupBox(tr("Session Details"), this);  // cn:会话详情
    detailGroup->setFixedWidth(250);
    QVBoxLayout* detailLayout = new QVBoxLayout(detailGroup);
    mDetailStack = new QStackedWidget(detailGroup);

    QLabel* placeholder = new QLabel(tr("Select a session to view details"), mDetailStack);  // cn:选中会话查看详情
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    mDetailStack->addWidget(placeholder);

    QWidget* detailPage = new QWidget(mDetailStack);
    QVBoxLayout* pageLayout = new QVBoxLayout(detailPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(6);

    mDetailTitleLabel = new QLabel(detailPage);
    QFont detailTitleFont = mDetailTitleLabel->font();
    detailTitleFont.setBold(true);
    mDetailTitleLabel->setFont(detailTitleFont);
    mDetailTitleLabel->setWordWrap(true);
    mDetailTitleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pageLayout->addWidget(mDetailTitleLabel);

    QFormLayout* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setContentsMargins(0, 0, 0, 0);
    mDetailStateValue   = new QLabel(detailPage);
    mDetailMsgValue     = new QLabel(detailPage);
    mDetailInValue      = new QLabel(detailPage);
    mDetailOutValue     = new QLabel(detailPage);
    mDetailTotalValue   = new QLabel(detailPage);
    mDetailCreatedValue = new QLabel(detailPage);
    mDetailUpdatedValue = new QLabel(detailPage);
    const QVector<QLabel*> valueLabels = { mDetailStateValue, mDetailMsgValue, mDetailInValue,
                                           mDetailOutValue, mDetailTotalValue,
                                           mDetailCreatedValue, mDetailUpdatedValue };
    for (QLabel* l : valueLabels) {
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
    form->addRow(tr("State"), mDetailStateValue);          // cn:状态
    form->addRow(tr("Messages"), mDetailMsgValue);         // cn:消息数
    form->addRow(tr("Input Tokens"), mDetailInValue);      // cn:输入 tokens
    form->addRow(tr("Output Tokens"), mDetailOutValue);    // cn:输出 tokens
    form->addRow(tr("Total Tokens"), mDetailTotalValue);   // cn:合计 tokens
    form->addRow(tr("Created"), mDetailCreatedValue);      // cn:创建时间
    form->addRow(tr("Updated"), mDetailUpdatedValue);      // cn:更新时间
    pageLayout->addLayout(form);
    pageLayout->addStretch(1);
    mDetailStack->addWidget(detailPage);

    detailLayout->addWidget(mDetailStack);
    mDetailStateDefaultColor = mDetailStateValue->palette().color(QPalette::WindowText);
    contentLayout->addWidget(detailGroup, 0);

    mainLayout->addLayout(contentLayout, 1);

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
    // 选中变化刷新按钮可用态与详情面板
    connect(mTable, &QTableWidget::itemSelectionChanged, this, &DADialogAgentSessionManager::onSelectionChanged);
    // 右键上下文菜单（切换/重命名/删除）
    connect(mTable, &QTableWidget::customContextMenuRequested, this, &DADialogAgentSessionManager::onTableContextMenu);

    updateButtonStates();
}

/**
 * @brief 填充会话表格
 * @param sessions 会话列表 payload
 *
 * 填充期间关闭排序避免逐项触发重排；填充完成后开启排序并按 id 定位选中当前会话。
 * 每行标题项携带整份 payload（RolePayload）供详情面板/汇总栏取数。
 */
void DADialogAgentSessionManager::populateSessions(const QVariantList& sessions)
{
    mTable->setSortingEnabled(false);
    mTable->setRowCount(0);  // 清空
    mTable->setRowCount(sessions.size());
    for (int i = 0; i < sessions.size(); ++i) {
        QVariantMap vm = sessions.at(i).toMap();
        QString id    = vm.value("id").toString();
        QString title = vm.value("title").toString();
        int msgCount  = vm.value("messageCount", 0).toInt();
        QString updatedIso = vm.value("updatedAt").toString();
        QString updated = formatTimestamp(updatedIso);

        // Title：空标题显示「(untitled)」
        QString displayTitle = title.isEmpty() ? tr("(untitled)")  // cn:（未命名）
                                                : title;
        // 跨工程会话（决策点 5）：标题附工程名后缀，明示"来自其它工程"
        if (vm.value(QStringLiteral("foreign")).toBool()) {
            const QString projName = QFileInfo(vm.value(QStringLiteral("projectPath")).toString()).fileName();
            displayTitle += QStringLiteral("  [%1]").arg(projName.isEmpty() ? tr("no project")  // cn:无工程
                                                                            : projName);
        }

        auto* titleItem = new SessionSortItem(displayTitle);
        titleItem->setData(RoleSessionId, id);
        titleItem->setData(RoleRawTitle, title);
        titleItem->setData(RolePayload, vm);

        // State（concurrent-sessions）：后台运行/等待输入/出错角标，空闲留空。
        // state 由 DAAgentModule::listSessionsForUI 附带：
        // starting/running/waiting_input/error，空串=空闲不显示。
        auto* stateItem = new SessionSortItem(QString());
        QString stateText;
        QColor stateColor;
        if (stateDisplay(vm.value("state").toString(), stateText, stateColor)) {
            stateItem->setText(stateText);
            stateItem->setForeground(stateColor);
        }
        stateItem->setTextAlignment(Qt::AlignCenter);

        auto* msgItem = new SessionSortItem(QString::number(msgCount));
        msgItem->setData(RoleSortValue, static_cast<qlonglong>(msgCount));
        msgItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto* updatedItem = new SessionSortItem(updated);
        updatedItem->setData(RoleSortValue, epochMsFromIso(updatedIso));
        updatedItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // 当前会话行加粗
        if (id == mCurrentSessionId) {
            QFont bold = titleItem->font();
            bold.setBold(true);
            titleItem->setFont(bold);
            stateItem->setFont(bold);
            msgItem->setFont(bold);
            updatedItem->setFont(bold);
        }

        mTable->setItem(i, ColTitle, titleItem);
        mTable->setItem(i, ColState, stateItem);
        mTable->setItem(i, ColMessages, msgItem);
        mTable->setItem(i, ColUpdated, updatedItem);
    }

    // 默认排序指示：更新时间倒序（与 store 返回顺序一致；关闭排序时设置只摆箭头不触发重排）
    mTable->horizontalHeader()->setSortIndicator(ColUpdated, Qt::DescendingOrder);
    mTable->setSortingEnabled(true);

    // 选中当前会话行：按 id 遍历定位（排序后行号不可靠），屏蔽信号避免副作用
    int selectRow = -1;
    if (!mCurrentSessionId.isEmpty()) {
        for (int r = 0; r < mTable->rowCount(); ++r) {
            if (sessionIdAt(r) == mCurrentSessionId) {
                selectRow = r;
                break;
            }
        }
    }
    if (selectRow >= 0) {
        mTable->blockSignals(true);
        mTable->selectRow(selectRow);
        mTable->blockSignals(false);
    }
    updateButtonStates();
    updateSummaryLabel();
    updateDetailPanel();
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
 * @brief 状态键 → 显示文本与语义色
 * @param stateKey listSessionsForUI 附带的运行态键
 * @param text 输出：显示文本
 * @param color 输出：语义色（对齐 icon-ui-design-guide 色板：绿=正常进行、金黄=需用户动作、橙红=错误）
 * @return 是否为非空闲状态（空闲返回 false，text/color 不置值）
 */
bool DADialogAgentSessionManager::stateDisplay(const QString& stateKey, QString& text, QColor& color)
{
    if (stateKey == QLatin1String("starting")) {
        text  = tr("Starting");         // cn:启动中
        color = QColor(0x66, 0x9E, 0x8B);
    } else if (stateKey == QLatin1String("running")) {
        text  = tr("Running");          // cn:运行中
        color = QColor(0x66, 0x9E, 0x8B);
    } else if (stateKey == QLatin1String("waiting_input")) {
        text  = tr("Waiting for you");  // cn:等待输入
        color = QColor(0xE6, 0xC2, 0x7C);
    } else if (stateKey == QLatin1String("error")) {
        text  = tr("Error");            // cn:出错
        color = QColor(0xCE, 0x60, 0x43);
    } else {
        return false;  // 空闲：表格列留空，详情面板由调用方显示「空闲」
    }
    return true;
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
 * @brief 注入跨工程存活会话（决策点 5 方案 c，审计问题 18；exec() 前调用）
 * @param sessions 每元素 QVariantMap{id,title,projectPath,state}
 */
void DADialogAgentSessionManager::setForeignSessions(const QVariantList& sessions)
{
    mForeignSessions = sessions;
    mAllProjectsCheck->setEnabled(!mForeignSessions.isEmpty());
    mAllProjectsCheck->setText(mForeignSessions.isEmpty()
        ? tr("Show sessions from all projects")  // cn:显示全部工程的会话
        : tr("Show sessions from all projects (%1 running in background)")  // cn:显示全部工程的会话（%1 个后台运行中）
              .arg(mForeignSessions.size()));
    if (mShowAllProjects) {
        populateSessions(combinedSessions());
    }
}

/**
 * @brief 合并列表：当前工程会话 +（勾选"全部工程"时）跨工程存活会话
 * @return 合并后的 payload 列表（foreign 条目附加 foreign=true 标记）
 */
QVariantList DADialogAgentSessionManager::combinedSessions() const
{
    QVariantList out = mBaseSessions;
    if (mShowAllProjects) {
        for (const QVariant& v : mForeignSessions) {
            QVariantMap vm = v.toMap();
            vm.insert(QStringLiteral("foreign"), true);
            out.append(vm);
        }
    }
    return out;
}

/**
 * @brief 指定行是否跨工程会话（payload foreign 标记）
 * @param row 行号
 * @return foreign 行返回 true（非法行 false）
 */
bool DADialogAgentSessionManager::isForeignRow(int row) const
{
    if (row < 0 || row >= mTable->rowCount()) return false;
    if (auto* it = mTable->item(row, ColTitle)) {
        return it->data(RolePayload).toMap().value(QStringLiteral("foreign")).toBool();
    }
    return false;
}

/**
 * @brief 根据当前选中刷新按钮可用态
 */
void DADialogAgentSessionManager::updateButtonStates()
{
    const int row = currentSelectedRow();
    bool hasSelection = (row >= 0);
    // 跨工程会话禁止切换（决策点 5，onSwitchClicked 同规）——按钮置灰
    mSwitchBtn->setEnabled(hasSelection && !isForeignRow(row));
    mRenameBtn->setEnabled(hasSelection);
    mDeleteBtn->setEnabled(hasSelection);
}

/**
 * @brief 根据当前选中刷新右侧详情面板
 *
 * 数据取自该行标题项携带的整份 payload（RolePayload）；
 * token 为 -1（旧数据未统计）时显示破折号；无选中切回占位页。
 */
void DADialogAgentSessionManager::updateDetailPanel()
{
    int row = currentSelectedRow();
    QTableWidgetItem* titleItem = (row >= 0) ? mTable->item(row, ColTitle) : nullptr;
    if (!titleItem) {
        mDetailStack->setCurrentIndex(0);  // 占位页
        return;
    }
    const QVariantMap vm = titleItem->data(RolePayload).toMap();

    // 标题（空标题显示「(untitled)」）
    QString title = vm.value("title").toString();
    mDetailTitleLabel->setText(title.isEmpty() ? tr("(untitled)")  // cn:（未命名）
                                                : title);

    // 状态（语义色；空闲显示「空闲」并恢复默认文字色）
    QString stateText;
    QColor stateColor;
    QPalette pal = mDetailStateValue->palette();
    if (stateDisplay(vm.value("state").toString(), stateText, stateColor)) {
        mDetailStateValue->setText(stateText);
        pal.setColor(QPalette::WindowText, stateColor);
    } else {
        mDetailStateValue->setText(tr("Idle"));  // cn:空闲
        pal.setColor(QPalette::WindowText, mDetailStateDefaultColor);
    }
    mDetailStateValue->setPalette(pal);

    mDetailMsgValue->setText(QString::number(vm.value("messageCount", 0).toInt()));
    mDetailInValue->setText(formatTokenDetail(vm.value("inputTokens", -1).toLongLong()));
    mDetailOutValue->setText(formatTokenDetail(vm.value("outputTokens", -1).toLongLong()));
    mDetailTotalValue->setText(formatTokenDetail(vm.value("totalTokens", -1).toLongLong()));
    mDetailCreatedValue->setText(formatTimestamp(vm.value("createdAt").toString()));
    mDetailUpdatedValue->setText(formatTimestamp(vm.value("updatedAt").toString()));

    mDetailStack->setCurrentIndex(1);  // 详情页
}

/**
 * @brief 刷新底部汇总栏（会话数 + 全部会话累计 tokens）
 */
void DADialogAgentSessionManager::updateSummaryLabel()
{
    const int count = mTable->rowCount();
    qint64 totalTokens = 0;
    for (int r = 0; r < count; ++r) {
        if (auto* it = mTable->item(r, ColTitle)) {
            qint64 t = it->data(RolePayload).toMap().value("totalTokens", -1).toLongLong();
            if (t > 0) totalTokens += t;  // -1（旧数据未统计）按 0 计
        }
    }
    mSummaryLabel->setText(tr("%1 sessions · %2 tokens total")  // cn:共 %1 个会话 · 累计 tokens %2
                              .arg(count)
                              .arg(formatTokenCount(totalTokens)));
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
    // 跨工程会话禁止切换（决策点 5）：会话列表按工程过滤，切入后该会话在
    // UI 立即"消失"（下拉/管理器均不可见）且工程边界语义混乱——只提供
    // 停止/删除/重命名
    if (isForeignRow(row)) return;
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
        // 本地乐观更新该行（含 payload 副本，保证详情面板同步显示新标题）
        titleItem->setData(RoleRawTitle, newTitle);
        titleItem->setText(newTitle);
        QVariantMap vm = titleItem->data(RolePayload).toMap();
        vm["title"] = newTitle;
        titleItem->setData(RolePayload, vm);
        emit renameRequested(sid, newTitle);
        updateDetailPanel();
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
        updateSummaryLabel();
        updateDetailPanel();
    }
}

/**
 * @brief 选中变化槽函数
 */
void DADialogAgentSessionManager::onSelectionChanged()
{
    updateButtonStates();
    updateDetailPanel();
}

/**
 * @brief 表格右键上下文菜单槽函数（切换/重命名/停止/删除，复用底部按钮逻辑）
 * @param pos 右键位置（viewport 坐标）
 */
void DADialogAgentSessionManager::onTableContextMenu(const QPoint& pos)
{
    QTableWidgetItem* it = mTable->itemAt(pos);
    if (!it) return;
    mTable->selectRow(it->row());  // 右键先选中该行
    QMenu menu(this);
    QAction* switchAct = menu.addAction(tr("Switch"));  // cn:切换
    switchAct->setEnabled(!isForeignRow(it->row()));  // 跨工程会话禁止切换（决策点 5）
    QAction* renameAct = menu.addAction(tr("Rename"));  // cn:重命名
    // 审计 L14：后台运行会话的停止入口——修复前失控后台会话必须先切换过去
    // 再按 Stop（结合问题 2 的切入冻结场景，starting 残留会话切过去也停不了）。
    // 按该行运行态启用（快照数据，对话框打开期间的新状态变化下次打开生效）
    QAction* stopAct = menu.addAction(tr("Stop"));  // cn:停止
    const QString state = mTable->item(it->row(), ColTitle)->data(RolePayload)
                              .toMap().value(QStringLiteral("state")).toString();
    stopAct->setEnabled(state == QLatin1String("running")
                        || state == QLatin1String("starting")
                        || state == QLatin1String("waiting_input"));
    menu.addSeparator();
    QAction* deleteAct = menu.addAction(tr("Delete"));  // cn:删除
    QAction* chosen = menu.exec(mTable->viewport()->mapToGlobal(pos));
    if (chosen == switchAct) {
        onSwitchClicked();
    } else if (chosen == renameAct) {
        onRenameClicked();
    } else if (chosen == stopAct) {
        emit stopRequested(sessionIdAt(it->row()));
    } else if (chosen == deleteAct) {
        onDeleteClicked();
    }
}

}  // namespace DA
