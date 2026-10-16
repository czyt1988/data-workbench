#include "DASettingPageShortcuts.h"
#include "DAShortcutManager.h"
#include "DALogCategory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QKeySequenceEdit>
#include <QHeaderView>
#include <QMenu>
#include <QShowEvent>
#include <QMap>
#include <algorithm>

namespace DA
{

DASettingPageShortcuts::DASettingPageShortcuts(QWidget* parent) : DAAbstractSettingPage(parent)
{
    buildUI();
}

DASettingPageShortcuts::~DASettingPageShortcuts()
{
}

/**
 * @brief 注入快捷键管理器
 *
 * 注入时不加载数据：设置对话框为缓存复用，数据在页面每次显示时经 showEvent 重新加载
 * @param mgr 快捷键管理器
 */
void DASettingPageShortcuts::setShortcutManager(DAShortcutManager* mgr)
{
    mManager = mgr;
}

/**
 * @brief 应用设置
 *
 * 把工作副本一次性提交管理器：立即写回全部 action 并落盘 shortcuts.json
 */
void DASettingPageShortcuts::apply()
{
    if (mManager) {
        mManager->setOverrides(mWorkingOverrides);
        mManager->applyAll();
        if (!mManager->save()) {
            daWarning.noquote() << tr("Failed to save shortcut settings");  // cn:保存快捷键设置失败
        }
    }
    Q_EMIT settingApplyed();
}

/**
 * @brief 设置页标题
 * @return 标题文本
 */
QString DASettingPageShortcuts::getSettingPageTitle() const
{
    return tr("Shortcuts");  // cn:快捷键
}

/**
 * @brief 设置页图标
 * @return 图标
 */
QIcon DASettingPageShortcuts::getSettingPageIcon() const
{
    return QIcon(QStringLiteral(":/DAGui/icon/setting-shortcuts.svg"));
}

/**
 * @brief 显示事件
 *
 * 每次显示时从管理器重载快照，丢弃上次取消对话框时未应用的修改
 * @param e 事件
 */
void DASettingPageShortcuts::showEvent(QShowEvent* e)
{
    DAAbstractSettingPage::showEvent(e);
    reloadFromManager();
}

/**
 * @brief 构建界面
 */
void DASettingPageShortcuts::buildUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(9, 9, 9, 9);
    mainLayout->setSpacing(6);

    // 顶部工具行：搜索过滤 + 全部重置
    QHBoxLayout* toolLayout = new QHBoxLayout();
    toolLayout->setSpacing(6);
    QLabel* searchLabel = new QLabel(tr("Search:"), this);  // cn:搜索：
    mSearchEdit         = new QLineEdit(this);
    mSearchEdit->setPlaceholderText(tr("Filter by command or shortcut"));  // cn:按命令或快捷键过滤
    mSearchEdit->setClearButtonEnabled(true);
    QPushButton* resetAllBtn = new QPushButton(tr("Reset All to Defaults"), this);  // cn:全部重置为默认
    toolLayout->addWidget(searchLabel);
    toolLayout->addWidget(mSearchEdit, 1);
    toolLayout->addWidget(resetAllBtn);
    mainLayout->addLayout(toolLayout);

    // 命令树：分类 -> 命令，第二列为快捷键录入框
    mTree = new QTreeWidget(this);
    mTree->setColumnCount(2);
    mTree->setHeaderLabels(QStringList() << tr("Command")   // cn:命令
                                         << tr("Shortcut"));  // cn:快捷键
    mTree->setRootIsDecorated(true);
    mTree->header()->setStretchLastSection(false);  // 快捷键列固定宽度，命令列拉伸
    mTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    mTree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    mTree->header()->resizeSection(1, 240);
    mTree->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(mTree, 1);

    // 底部状态提示（冲突等信息，错误态用橙红 #CE6043）
    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(true);
    mStatusLabel->setVisible(false);
    mainLayout->addWidget(mStatusLabel);

    connect(mSearchEdit, &QLineEdit::textChanged, this, &DASettingPageShortcuts::onSearchTextChanged);
    connect(resetAllBtn, &QPushButton::clicked, this, &DASettingPageShortcuts::onResetAllClicked);
    connect(mTree, &QTreeWidget::customContextMenuRequested, this, &DASettingPageShortcuts::onTreeContextMenu);
}

/**
 * @brief 从管理器重新加载工作副本并重建命令树
 */
void DASettingPageShortcuts::reloadFromManager()
{
    mWorkingOverrides = mManager ? mManager->overrides() : QHash< QString, QKeySequence >();
    rebuildTree();
    applyFilter();
    clearStatus();
}

/**
 * @brief 重建命令树
 *
 * 分类按固定顺序排列（主页/数据/绘图/图表操作/视图/工作流/工作流编辑器/其它），
 * 分类内命令按显示名本地化排序
 */
void DASettingPageShortcuts::rebuildTree()
{
    mTree->clear();
    mCommandItems.clear();
    mEditors.clear();
    if (nullptr == mManager) {
        return;
    }
    // 按分类分组
    QMap< QString, QStringList > grouped;
    const QStringList cmds = mManager->commandNames();
    for (const QString& objName : cmds) {
        grouped[ mManager->categoryOf(objName) ].append(objName);
    }
    // 分类显示顺序：固定顺序表优先，未登记的分类排在最后
    static const QStringList catOrder = { QStringLiteral("Main"),
                                          QStringLiteral("Data"),
                                          QStringLiteral("Figure"),
                                          QStringLiteral("Chart"),
                                          QStringLiteral("View"),
                                          QStringLiteral("Workflow"),
                                          QStringLiteral("WorkflowEditor"),
                                          QStringLiteral("Other") };
    QStringList orderedCats;
    for (const QString& c : catOrder) {
        if (grouped.contains(c)) {
            orderedCats.append(c);
        }
    }
    for (auto it = grouped.constBegin(); it != grouped.constEnd(); ++it) {
        if (!catOrder.contains(it.key())) {
            orderedCats.append(it.key());
        }
    }

    for (const QString& catKey : std::as_const(orderedCats)) {
        QTreeWidgetItem* catItem = new QTreeWidgetItem(mTree);
        catItem->setText(0, categoryDisplayName(catKey));
        QFont catFont = catItem->font(0);
        catFont.setBold(true);
        catItem->setFont(0, catFont);
        catItem->setFlags(Qt::ItemIsEnabled);  // 分类节点不可选中
        // 分类内按显示名排序
        QStringList names = grouped.value(catKey);
        std::sort(names.begin(), names.end(), [this](const QString& a, const QString& b) {
            return QString::localeAwareCompare(mManager->displayNameOf(a), mManager->displayNameOf(b)) < 0;
        });
        for (const QString& objName : std::as_const(names)) {
            QTreeWidgetItem* item = new QTreeWidgetItem(catItem);
            item->setData(0, Qt::UserRole, objName);
            item->setToolTip(0, objName);
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            QKeySequenceEdit* editor = new QKeySequenceEdit(mTree);
            editor->setMaximumSequenceLength(1);
            editor->setMinimumWidth(180);
            mTree->setItemWidget(item, 1, editor);
            mCommandItems.insert(objName, item);
            mEditors.insert(objName, editor);
            connect(editor, &QKeySequenceEdit::keySequenceChanged, this,
                    [this, objName](const QKeySequence& keys) {
                        onEditorChanged(objName, keys);
                    });
            refreshRow(objName);
        }
    }
    mTree->collapseAll();
}

/**
 * @brief 工作副本下的生效快捷键
 * @param objName 命令 ID
 * @return 生效键序列（覆盖优先，空覆盖=显式清除）
 */
QKeySequence DASettingPageShortcuts::workingKeysOf(const QString& objName) const
{
    auto it = mWorkingOverrides.constFind(objName);
    if (it != mWorkingOverrides.constEnd()) {
        return it.value();
    }
    return mManager ? mManager->defaultKeysOf(objName) : QKeySequence();
}

/**
 * @brief 工作副本下与 keys 冲突的其它命令 ID
 * @param excludeObjName 排除的命令（正在录入的命令自身）
 * @param keys 待校验键序列
 * @return 冲突命令 ID 列表
 */
QStringList DASettingPageShortcuts::conflictingInWorking(const QString& excludeObjName, const QKeySequence& keys) const
{
    QStringList res;
    if (keys.isEmpty() || nullptr == mManager) {
        return res;
    }
    const QStringList cmds = mManager->commandNames();
    for (const QString& c : cmds) {
        if (c == excludeObjName) {
            continue;
        }
        if (DAShortcutManager::keySequencesConflict(keys, workingKeysOf(c))) {
            res.append(c);
        }
    }
    return res;
}

/**
 * @brief 快捷键录入槽
 *
 * 冲突时阻止录入：恢复编辑器原值、该行红色标记并提示占用命令；
 * 录入值与默认一致时撤销覆盖（保持稀疏存储干净）
 * @param objName 命令 ID
 * @param keys 新录入的键序列
 */
void DASettingPageShortcuts::onEditorChanged(const QString& objName, const QKeySequence& keys)
{
    if (mUpdating || nullptr == mManager) {
        return;
    }
    const QKeySequence prev = workingKeysOf(objName);
    if (keys == prev) {
        return;
    }
    const QStringList conflicts = conflictingInWorking(objName, keys);
    if (!conflicts.isEmpty()) {
        // 阻止录入：恢复原值并红色提示
        QStringList conflictNames;
        for (const QString& c : conflicts) {
            conflictNames.append(mManager->displayNameOf(c));
        }
        showStatus(tr("Shortcut %1 is already assigned to: %2")  // cn:快捷键 %1 已被命令占用：%2
                       .arg(keys.toString(QKeySequence::NativeText), conflictNames.join(QStringLiteral("、"))),
                   true);
        QKeySequenceEdit* editor = mEditors.value(objName, nullptr);
        if (editor) {
            mUpdating = true;
            editor->setKeySequence(prev);
            mUpdating = false;
        }
        markRowError(objName, true);
        return;
    }
    markRowError(objName, false);
    if (keys == mManager->defaultKeysOf(objName)) {
        // 与默认一致：撤销覆盖
        mWorkingOverrides.remove(objName);
    } else {
        mWorkingOverrides.insert(objName, keys);
    }
    refreshRow(objName);
    showStatus(tr("Shortcut for \"%1\" changed, click OK or Apply to take effect")  // cn:命令“%1”的快捷键已修改，点击确定或应用后生效
                   .arg(mManager->displayNameOf(objName)),
               false);
    Q_EMIT settingChanged();
}

/**
 * @brief 刷新单行显示
 * @param objName 命令 ID
 */
void DASettingPageShortcuts::refreshRow(const QString& objName)
{
    QTreeWidgetItem* item   = mCommandItems.value(objName, nullptr);
    QKeySequenceEdit* editor = mEditors.value(objName, nullptr);
    if (nullptr == item || nullptr == editor || nullptr == mManager) {
        return;
    }
    const QKeySequence def = mManager->defaultKeysOf(objName);
    const QKeySequence eff = workingKeysOf(objName);
    const bool modified    = mWorkingOverrides.contains(objName);
    mUpdating              = true;
    editor->setKeySequence(eff);
    mUpdating = false;
    item->setText(0, mManager->displayNameOf(objName));
    QFont f = item->font(0);
    f.setBold(modified);  // 已修改的行加粗标记
    item->setFont(0, f);
    if (modified) {
        item->setToolTip(1, tr("Default: %1")  // cn:默认：%1
                                .arg(def.isEmpty() ? tr("(none)") : def.toString(QKeySequence::NativeText)));
    } else {
        item->setToolTip(1, QString());
    }
}

/**
 * @brief 刷新全部行显示
 */
void DASettingPageShortcuts::refreshAllRows()
{
    for (auto it = mCommandItems.constBegin(); it != mCommandItems.constEnd(); ++it) {
        refreshRow(it.key());
        markRowError(it.key(), false);
    }
}

/**
 * @brief 冲突红色标记
 * @param objName 命令 ID
 * @param isError 是否标记为错误（橙红 #CE6043）
 */
void DASettingPageShortcuts::markRowError(const QString& objName, bool isError)
{
    QTreeWidgetItem* item = mCommandItems.value(objName, nullptr);
    if (nullptr == item) {
        return;
    }
    if (isError) {
        const QBrush errBrush(QColor(0xCE, 0x60, 0x43));
        item->setForeground(0, errBrush);
        item->setForeground(1, errBrush);
    } else {
        item->setForeground(0, QBrush());
        item->setForeground(1, QBrush());
    }
}

/**
 * @brief 分类显示名
 *
 * 分类键为收集器写入的稳定英文标识，此处翻译为界面文本
 * @param catKey 分类键
 * @return 显示名
 */
QString DASettingPageShortcuts::categoryDisplayName(const QString& catKey) const
{
    if (catKey == QLatin1String("Main")) {
        return tr("Main");  // cn:主页
    }
    if (catKey == QLatin1String("Data")) {
        return tr("Data");  // cn:数据
    }
    if (catKey == QLatin1String("Figure")) {
        return tr("Figure");  // cn:绘图
    }
    if (catKey == QLatin1String("Chart")) {
        return tr("Chart");  // cn:图表操作
    }
    if (catKey == QLatin1String("View")) {
        return tr("View");  // cn:视图
    }
    if (catKey == QLatin1String("Workflow")) {
        return tr("Workflow");  // cn:工作流
    }
    if (catKey == QLatin1String("WorkflowEditor")) {
        return tr("Workflow Editor");  // cn:工作流编辑器
    }
    if (catKey == QLatin1String("Other")) {
        return tr("Other");  // cn:其它
    }
    return catKey;
}

/**
 * @brief 底部状态提示
 * @param text 提示文本
 * @param isError 是否错误态（橙红 #CE6043，否则深蓝灰正文色）
 */
void DASettingPageShortcuts::showStatus(const QString& text, bool isError)
{
    if (nullptr == mStatusLabel) {
        return;
    }
    mStatusLabel->setText(text);
    mStatusLabel->setStyleSheet(isError ? QStringLiteral("color: #CE6043;") : QStringLiteral("color: #5D6D7E;"));
    mStatusLabel->setVisible(true);
}

/**
 * @brief 清除底部状态提示
 */
void DASettingPageShortcuts::clearStatus()
{
    if (mStatusLabel) {
        mStatusLabel->clear();
        mStatusLabel->setVisible(false);
    }
}

/**
 * @brief 搜索框内容变化
 * @param text 过滤文本
 */
void DASettingPageShortcuts::onSearchTextChanged(const QString& text)
{
    Q_UNUSED(text);
    applyFilter();
}

/**
 * @brief 按搜索框内容过滤命令树
 *
 * 匹配命令显示名、命令 ID 或生效快捷键文本；过滤时展开命中的分类，清空过滤时收起全部分类
 */
void DASettingPageShortcuts::applyFilter()
{
    if (nullptr == mTree || nullptr == mManager) {
        return;
    }
    const QString filter = mSearchEdit ? mSearchEdit->text().trimmed() : QString();
    for (int i = 0; i < mTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* catItem = mTree->topLevelItem(i);
        int visibleCount         = 0;
        for (int j = 0; j < catItem->childCount(); ++j) {
            QTreeWidgetItem* item  = catItem->child(j);
            const QString objName  = item->data(0, Qt::UserRole).toString();
            bool match             = true;
            if (!filter.isEmpty()) {
                const QKeySequence eff = workingKeysOf(objName);
                match                  = mManager->displayNameOf(objName).contains(filter, Qt::CaseInsensitive)
                        || objName.contains(filter, Qt::CaseInsensitive)
                        || eff.toString(QKeySequence::NativeText).contains(filter, Qt::CaseInsensitive);
            }
            item->setHidden(!match);
            if (match) {
                ++visibleCount;
            }
        }
        catItem->setHidden(0 == visibleCount);
        catItem->setExpanded(!filter.isEmpty());
    }
}

/**
 * @brief 全部重置为默认
 */
void DASettingPageShortcuts::onResetAllClicked()
{
    mWorkingOverrides.clear();
    refreshAllRows();
    clearStatus();
    Q_EMIT settingChanged();
}

/**
 * @brief 命令树右键菜单：重置为默认 / 清除快捷键
 * @param pos 视口坐标
 */
void DASettingPageShortcuts::onTreeContextMenu(const QPoint& pos)
{
    if (nullptr == mManager) {
        return;
    }
    QTreeWidgetItem* item = mTree->itemAt(pos);
    if (nullptr == item || nullptr == item->parent()) {
        return;  // 分类节点无菜单
    }
    const QString objName = item->data(0, Qt::UserRole).toString();
    QMenu menu(this);
    QAction* resetAct = menu.addAction(tr("Reset to Default"));  // cn:重置为默认
    resetAct->setEnabled(mWorkingOverrides.contains(objName));
    QAction* clearAct = menu.addAction(tr("Clear Shortcut"));  // cn:清除快捷键
    clearAct->setEnabled(!workingKeysOf(objName).isEmpty());
    QAction* selected = menu.exec(mTree->viewport()->mapToGlobal(pos));
    if (selected == resetAct) {
        mWorkingOverrides.remove(objName);
    } else if (selected == clearAct) {
        if (mManager->defaultKeysOf(objName).isEmpty()) {
            // 默认即为空：清除等价于撤销覆盖
            mWorkingOverrides.remove(objName);
        } else {
            mWorkingOverrides.insert(objName, QKeySequence());  // 显式清除
        }
    } else {
        return;
    }
    refreshRow(objName);
    markRowError(objName, false);
    clearStatus();
    Q_EMIT settingChanged();
}

}  // namespace DA
