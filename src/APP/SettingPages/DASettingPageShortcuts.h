#ifndef DASETTINGPAGESHORTCUTS_H
#define DASETTINGPAGESHORTCUTS_H
#include "DAAbstractSettingPage.h"
#include <QKeySequence>
#include <QHash>

class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QLineEdit;
class QLabel;
class QKeySequenceEdit;

namespace DA
{
class DAShortcutManager;
class DAStringFilterProxyModel;

/**
 * @brief 快捷键设置页
 *
 * 按分类树展示全部可配置命令，支持搜索过滤、录入自定义快捷键（与其它命令冲突时阻止录入并红色提示）、
 * 单命令「重置为默认/清除快捷键」与「全部重置为默认」。
 *
 * 页面改动先存本地工作副本，点「确定/应用」时一次性提交 DAShortcutManager（立即生效并落盘
 * shortcuts.json）；设置对话框为缓存复用，页面每次显示时经 showEvent 从管理器重载快照，
 * 因此取消对话框会丢弃未应用的修改
 */
class DASettingPageShortcuts : public DAAbstractSettingPage
{
    Q_OBJECT
public:
    DASettingPageShortcuts(QWidget* parent = nullptr);
    ~DASettingPageShortcuts();
    // 注入快捷键管理器（注入时不加载数据，页面显示时重新加载快照）
    void setShortcutManager(DAShortcutManager* mgr);
    // 应用设置：提交工作副本到管理器并落盘
    virtual void apply() override;
    // 设置页标题
    virtual QString getSettingPageTitle() const override;
    // 设置页图标
    virtual QIcon getSettingPageIcon() const override;

protected:
    // 每次显示时从管理器重载快照（丢弃取消时未应用的修改）
    virtual void showEvent(QShowEvent* e) override;

private Q_SLOTS:
    void onSearchTextChanged(const QString& text);
    void onResetAllClicked();
    void onTreeContextMenu(const QPoint& pos);
    void onEditorChanged(const QString& objName, const QKeySequence& keys);

private:
    void buildUI();
    // 从管理器重新加载工作副本并重建命令树
    void reloadFromManager();
    void rebuildTree();
    // 工作副本下的生效快捷键
    QKeySequence workingKeysOf(const QString& objName) const;
    // 工作副本下与 keys 冲突的其它命令 ID
    QStringList conflictingInWorking(const QString& excludeObjName, const QKeySequence& keys) const;
    // 刷新单行/全部行显示（编辑器值、已修改加粗、默认值提示）
    void refreshRow(const QString& objName);
    void refreshAllRows();
    // 冲突红色标记
    void markRowError(const QString& objName, bool isError);
    // 分类显示名（分类键为稳定英文标识，此处翻译）
    QString categoryDisplayName(const QString& catKey) const;
    // 底部状态提示
    void showStatus(const QString& text, bool isError);
    void clearStatus();
    // 按搜索框内容过滤命令树
    void applyFilter();

private:
    DAShortcutManager* mManager { nullptr };
    QTreeView* mTree { nullptr };
    QStandardItemModel* mModel { nullptr };
    DAStringFilterProxyModel* mProxy { nullptr };
    QLineEdit* mSearchEdit { nullptr };
    QLabel* mStatusLabel { nullptr };
    QHash< QString, QKeySequence > mWorkingOverrides;  ///< 工作副本（键存在=已覆盖，空序列=显式清除）
    QHash< QString, QStandardItem* > mCommandItems;
    QHash< QString, QKeySequenceEdit* > mEditors;
    bool mUpdating { false };  ///< 程序性刷新编辑器时抑制 keySequenceChanged 处理
};
}
#endif  // DASETTINGPAGESHORTCUTS_H
