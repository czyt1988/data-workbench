#include "DAPluginManagerDialog.h"
#include "ui_DAPluginManagerDialog.h"
#include <QTreeWidgetItem>
#include <QDebug>
#include "DAAbstractNodePlugin.h"
#include "DAAppPluginManager.h"
#include "DAPluginFileInfo.h"
#include "DALogCategory.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

namespace
{
// 列表项数据角色
const int RoleBaseName        = Qt::UserRole + 1;  ///< C++插件为文件基本名，Python包为目录名
const int RoleIsPyPlugin      = Qt::UserRole + 2;  ///< bool，是否Python节点包
const int RoleCurrentEnabled  = Qt::UserRole + 3;  ///< bool，当前生效的启用状态（勾选变更的比对基准）
}

//===================================================
// DAPluginManagerDialog
//===================================================
DAPluginManagerDialog::DAPluginManagerDialog(DAAppPluginManager* mgr, QWidget* parent)
    : QDialog(parent), ui(new Ui::DAPluginManagerDialog), mPluginMgr(mgr)
{
    ui->setupUi(this);
    init();
}

DAPluginManagerDialog::~DAPluginManagerDialog()
{
    delete ui;
}

void DAPluginManagerDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        updatePluginList();
        break;

    default:
        break;
    }
}

void DAPluginManagerDialog::accept()
{
    // 确定=应用全部勾选变更后关闭
    applyChanges();
    QDialog::accept();
}

void DAPluginManagerDialog::onApplyClicked()
{
    applyChanges();
}

void DAPluginManagerDialog::init()
{
    ui->treeWidget->setColumnCount(4);
    ui->treeWidget->setHeaderLabels({ tr("Name"), tr("Version"), tr("Status"), tr("Description") });  // cn:名称,版本,状态,描述
    ui->treeWidget->setUniformRowHeights(true);
    connect(ui->pushButtonApply, &QPushButton::clicked, this, &DAPluginManagerDialog::onApplyClicked);
    updatePluginList();
    ui->treeWidget->expandAll();
}

/**
 * @brief 重建插件列表
 *
 * C++插件组来自scanPluginFiles（含已加载、禁用、加载失败、降级停用等状态），
 * Python节点包组来自getPyPluginPackageInfos。勾选状态与当前生效状态一致，
 * 用户勾选后由applyChanges统一执行变更
 */
void DAPluginManagerDialog::updatePluginList()
{
    if (nullptr == mPluginMgr) {
        return;
    }
    ui->treeWidget->clear();
    //----------------------------------------------------
    // C++ 插件组
    //----------------------------------------------------
    QTreeWidgetItem* cppGroup = new QTreeWidgetItem(ui->treeWidget);
    cppGroup->setText(0, tr("C++ Plugins"));  // cn:C++插件
    const QList< DAPluginFileInfo > infos = mPluginMgr->scanPluginFiles();
    for (const DAPluginFileInfo& info : infos) {
        QTreeWidgetItem* item = new QTreeWidgetItem(cppGroup);
        item->setText(0, info.name);
        item->setData(0, RoleBaseName, info.baseName);
        item->setData(0, RoleIsPyPlugin, false);
        item->setData(0, RoleCurrentEnabled, info.state == DAPluginFileInfo::Loaded);
        item->setCheckState(0, info.state == DAPluginFileInfo::Loaded ? Qt::Checked : Qt::Unchecked);
        item->setText(1, info.version);
        QString status;
        switch (info.state) {
        case DAPluginFileInfo::Loaded:
            status = tr("Loaded");  // cn:已加载
            break;
        case DAPluginFileInfo::NotLoaded:
            status = tr("Not Loaded");  // cn:未加载
            break;
        case DAPluginFileInfo::Disabled:
            status = tr("Disabled");  // cn:已禁用
            break;
        case DAPluginFileInfo::LoadFailed:
            status = tr("Load Failed");  // cn:加载失败
            break;
        case DAPluginFileInfo::InactivePendingRestart:
            status = tr("Inactive (restart to release library)");  // cn:已停用（重启后释放库）
            break;
        }
        item->setText(2, status);
        item->setText(3, info.description);
        if (!info.errorString.isEmpty()) {
            item->setToolTip(2, info.errorString);
        }
    }
    //----------------------------------------------------
    // Python 节点包组
    //----------------------------------------------------
    QTreeWidgetItem* pyGroup = new QTreeWidgetItem(ui->treeWidget);
    pyGroup->setText(0, tr("Python Node Packages (changes take effect after restart)"));  // cn:Python节点包（变更重启后生效）
    const QList< DAPyPluginPackageInfo > pyInfos = mPluginMgr->getPyPluginPackageInfos();
    for (const DAPyPluginPackageInfo& info : pyInfos) {
        QTreeWidgetItem* item = new QTreeWidgetItem(pyGroup);
        item->setText(0, info.packageName);
        item->setData(0, RoleBaseName, info.packageName);
        item->setData(0, RoleIsPyPlugin, true);
        item->setData(0, RoleCurrentEnabled, info.enabled);
        item->setCheckState(0, info.enabled ? Qt::Checked : Qt::Unchecked);
        item->setText(2, info.enabled ? tr("Enabled") : tr("Disabled"));  // cn:已启用 / 已禁用
    }
    ui->treeWidget->expandAll();
}

/**
 * @brief 应用全部勾选变更
 *
 * 逐行比对勾选状态与当前生效状态：
 * - C++插件：勾选→enablePlugin热加载，取消→disablePlugin热卸载（含在用节点守卫），
 *   结果持久化到plugins/.pluginignore
 * - Python节点包：写pyplugins/.pluginignore，下次启动生效
 * 失败项经daWarning提示并保持原状态，完成后刷新列表
 */
void DAPluginManagerDialog::applyChanges()
{
    if (nullptr == mPluginMgr) {
        return;
    }
    QStringList errors;
    int changedCount   = 0;
    bool needRestart   = false;
    const int groupCnt = ui->treeWidget->topLevelItemCount();
    for (int g = 0; g < groupCnt; ++g) {
        QTreeWidgetItem* group = ui->treeWidget->topLevelItem(g);
        for (int i = 0; i < group->childCount(); ++i) {
            QTreeWidgetItem* item   = group->child(i);
            const bool checked      = (item->checkState(0) == Qt::Checked);
            const bool current      = item->data(0, RoleCurrentEnabled).toBool();
            if (checked == current) {
                continue;
            }
            const bool isPy         = item->data(0, RoleIsPyPlugin).toBool();
            const QString baseName  = item->data(0, RoleBaseName).toString();
            if (isPy) {
                if (mPluginMgr->setPyPluginEnabled(baseName, checked)) {
                    ++changedCount;
                    needRestart = true;
                } else {
                    errors << tr("Failed to save the enabled state of Python package %1").arg(baseName);  // cn:保存Python包%1的启用状态失败
                }
            } else {
                QString err;
                const bool ok = checked ? mPluginMgr->enablePlugin(baseName, &err) : mPluginMgr->disablePlugin(baseName, &err);
                if (ok) {
                    ++changedCount;
                } else {
                    errors << err;
                }
            }
        }
    }
    if (changedCount > 0) {
        // 检查是否有降级停用的插件（库释放失败，重启后完全释放）
        const QList< DAPluginFileInfo > infos = mPluginMgr->scanPluginFiles();
        for (const DAPluginFileInfo& info : infos) {
            if (info.state == DAPluginFileInfo::InactivePendingRestart) {
                needRestart = true;
                break;
            }
        }
    }
    updatePluginList();
    if (!errors.isEmpty()) {
        daWarning << errors.join(QLatin1Char('\n'));
    }
    if (changedCount > 0) {
        if (needRestart) {
            daInfo << tr("%1 plugin change(s) applied, some changes take effect after restart")
                          .arg(changedCount);  // cn:已应用%1项插件变更，部分变更将在重启后生效
        } else {
            daInfo << tr("%1 plugin change(s) applied").arg(changedCount);  // cn:已应用%1项插件变更
        }
    }
}
