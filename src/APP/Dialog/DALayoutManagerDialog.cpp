#include "DALayoutManagerDialog.h"
#include "DAAppLayoutManager.h"
#include "DALogCategory.h"
// Qt
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>

namespace DA
{
//===================================================
// DALayoutManagerDialog
//===================================================

DALayoutManagerDialog::DALayoutManagerDialog(DAAppLayoutManager* mgr, QWidget* parent)
    : QDialog(parent), m_mgr(mgr)
{
    buildUi();
    refreshList();
    setWindowTitle(tr("Layout Manager"));  // cn:布局管理
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

/**
 * @brief 构建界面：左侧方案列表 + 右侧操作按钮 + 底部关闭
 */
void DALayoutManagerDialog::buildUi()
{
    resize(460, 360);
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    auto* topLayout = new QHBoxLayout();
    topLayout->setSpacing(8);

    // 左侧方案列表
    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_list->setToolTip(tr("Double-click a scheme to apply it"));  // cn:双击方案可直接应用
    topLayout->addWidget(m_list, 1);

    // 右侧按钮区
    auto* btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(8);
    m_saveCurrentBtn = new QPushButton(QIcon(":/app/bright/Icon/save.svg"), tr("Save Current"), this);  // cn:保存当前布局
    m_applyBtn       = new QPushButton(QIcon(":/app/bright/Icon/run.svg"), tr("Apply Selected"), this);  // cn:应用选中布局
    m_deleteBtn      = new QPushButton(QIcon(":/app/bright/Icon/removeData.svg"), tr("Delete Selected"), this);  // cn:删除选中布局
    m_resetDefaultBtn = new QPushButton(QIcon(":/app/bright/Icon/viewAll.svg"), tr("Reset Default"), this);  // cn:恢复默认布局
    m_saveCurrentBtn->setToolTip(tr("Save the current window layout as a named scheme"));  // cn:把当前窗口布局保存为命名方案
    m_applyBtn->setToolTip(tr("Apply the selected layout scheme"));  // cn:应用选中的布局方案
    m_deleteBtn->setToolTip(tr("Delete the selected custom layout scheme (presets cannot be deleted)"));  // cn:删除选中的自定义布局方案（预置方案不可删除）
    m_resetDefaultBtn->setToolTip(tr("Restore the default window layout"));  // cn:恢复默认窗口布局
    btnLayout->addWidget(m_saveCurrentBtn);
    btnLayout->addWidget(m_applyBtn);
    btnLayout->addWidget(m_deleteBtn);
    btnLayout->addWidget(m_resetDefaultBtn);
    btnLayout->addStretch(1);
    topLayout->addLayout(btnLayout);

    mainLayout->addLayout(topLayout, 1);

    // 底部关闭
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch(1);
    auto* closeBtn = new QPushButton(tr("Close"), this);  // cn:关闭
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);

    connect(m_saveCurrentBtn, &QPushButton::clicked, this, &DALayoutManagerDialog::onSaveCurrentClicked);
    connect(m_applyBtn, &QPushButton::clicked, this, &DALayoutManagerDialog::onApplyClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &DALayoutManagerDialog::onDeleteClicked);
    connect(m_resetDefaultBtn, &QPushButton::clicked, this, &DALayoutManagerDialog::onResetDefaultClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &DALayoutManagerDialog::onItemDoubleClicked);
    connect(m_list, &QListWidget::itemSelectionChanged, this, &DALayoutManagerDialog::onSelectionChanged);
}

/**
 * @brief 重新读取方案列表填充（预置在前，预置显示翻译名）
 */
void DALayoutManagerDialog::refreshList()
{
    m_list->clear();
    if (!m_mgr) {
        return;
    }
    const QStringList names = m_mgr->layoutNames();
    for (const QString& n : names) {
        QString display;
        if (n == QStringLiteral("Default")) {
            display = tr("Default");  // cn:默认布局
        } else if (n == QStringLiteral("Focus Analysis")) {
            display = tr("Focus Analysis");  // cn:专注分析
        } else {
            display = n;
        }
        auto* item = new QListWidgetItem(display, m_list);
        item->setData(Qt::UserRole, n);
        m_list->addItem(item);
    }
    updateButtonStates();
}

/**
 * @brief 按选中状态刷新按钮可用性（无选中禁用应用/删除，预置方案不可删除）
 */
void DALayoutManagerDialog::updateButtonStates()
{
    const QString name = currentSelectedName();
    const bool hasSelection = !name.isEmpty();
    m_applyBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection && m_mgr && !m_mgr->isPreset(name));
}

/**
 * @brief 当前选中方案的原始名
 */
QString DALayoutManagerDialog::currentSelectedName() const
{
    QListWidgetItem* item = m_list->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

/**
 * @brief 保存当前布局为命名方案（弹命名输入框，同名覆盖，预置名拒绝）
 */
void DALayoutManagerDialog::onSaveCurrentClicked()
{
    if (!m_mgr) {
        return;
    }
    bool ok     = false;
    QString name = QInputDialog::getText(this,
                                         tr("Save Layout"),          // cn:保存布局方案
                                         tr("Layout scheme name:"),  // cn:布局方案名称：
                                         QLineEdit::Normal,
                                         QString(),
                                         &ok);
    if (!ok || name.isEmpty()) {
        return;
    }
    if (m_mgr->isPreset(name)) {
        daWarning << tr("Cannot overwrite preset layout schemes, please use another name");  // cn:不能覆盖预置布局方案，请换一个名称
        return;
    }
    if (m_mgr->saveLayoutAs(name)) {
        daInfo << tr("Layout scheme '%1' saved").arg(name);  // cn:布局方案"%1"已保存
        refreshList();
        // 选中新保存的方案，便于后续直接应用
        for (int i = 0; i < m_list->count(); ++i) {
            if (m_list->item(i)->data(Qt::UserRole).toString() == name) {
                m_list->setCurrentRow(i);
                break;
            }
        }
    }
}

/**
 * @brief 应用选中布局
 */
void DALayoutManagerDialog::onApplyClicked()
{
    applyLayout(currentSelectedName());
}

/**
 * @brief 删除选中布局（预置不可删，删除前确认）
 */
void DALayoutManagerDialog::onDeleteClicked()
{
    if (!m_mgr) {
        return;
    }
    const QString name = currentSelectedName();
    if (name.isEmpty() || m_mgr->isPreset(name)) {
        return;
    }
    if (QMessageBox::question(this,
                              tr("Remove Layout"),                          // cn:删除布局方案
                              tr("Remove layout scheme '%1'?").arg(name),   // cn:确定删除布局方案"%1"吗？
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No)
        != QMessageBox::Yes) {
        return;
    }
    if (m_mgr->removeLayout(name)) {
        daInfo << tr("Layout scheme '%1' removed").arg(name);  // cn:布局方案"%1"已删除
        refreshList();
    }
}

/**
 * @brief 恢复默认布局（弹确认框）
 */
void DALayoutManagerDialog::onResetDefaultClicked()
{
    if (!m_mgr) {
        return;
    }
    if (QMessageBox::question(this,
                              tr("Reset Layout"),  // cn:恢复默认布局
                              tr("This will restore the default window layout. Continue?"),  // cn:将恢复默认窗口布局，是否继续？
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No)
        != QMessageBox::Yes) {
        return;
    }
    applyLayout(QStringLiteral("Default"));
}

/**
 * @brief 双击方案项直接应用
 */
void DALayoutManagerDialog::onItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) {
        return;
    }
    applyLayout(item->data(Qt::UserRole).toString());
}

/**
 * @brief 选中项变化时刷新按钮状态
 */
void DALayoutManagerDialog::onSelectionChanged()
{
    updateButtonStates();
}

/**
 * @brief 应用指定方案（保持对话框打开，便于连续尝试不同方案）
 */
void DALayoutManagerDialog::applyLayout(const QString& name)
{
    if (!m_mgr || name.isEmpty()) {
        return;
    }
    m_mgr->openLayout(name);
}

}  // namespace DA
