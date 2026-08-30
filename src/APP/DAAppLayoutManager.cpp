#include "DAAppLayoutManager.h"
#include "AppMainWindow.h"
#include "DAAppDockingArea.h"
#include "DAAbstractSettingPage.h"
#include "DALogCategory.h"
// Qt
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QInputDialog>
#include <QUndoStack>
// Qt-Advanced-Docking-System
#include "DockManager.h"

namespace DA
{
//===================================================
// DAAppLayoutManager
//===================================================

DAAppLayoutManager::DAAppLayoutManager(AppMainWindow* mw, QObject* par) : QObject(par), mMainWindow(mw)
{
}

DAAppLayoutManager::~DAAppLayoutManager()
{
}

/**
 * @brief 初始化布局方案管理器
 *
 * 预置方案「Default」直接引用启动快照（与恢复默认布局同一字节流）；
 * 「Focus Analysis」在默认布局基础上隐藏左右侧栏后生成，仅在首次初始化时计算
 * @param defaultSnapshot AppMainWindow 构造时抓取的默认快照
 */
void DAAppLayoutManager::initialize(const QByteArray& defaultSnapshot)
{
    mDefaultSnapshot = defaultSnapshot;
    loadCustomLayouts();
}

/**
 * @brief 保存当前布局为自定义方案（同名覆盖）
 * @param name 方案名
 * @return 成功返回 true
 */
bool DAAppLayoutManager::saveLayoutAs(const QString& name)
{
    if (name.isEmpty() || isPreset(name)) {
        return false;
    }
    mCustom[ name ] = mMainWindow->saveUIState();
    if (!mCustomOrder.contains(name)) {
        mCustomOrder.append(name);
    }
    saveCustomLayouts();
    return true;
}

/**
 * @brief 打开（应用）方案
 *
 * 预置 Default 用启动快照；Focus Analysis 现场从默认快照恢复后隐藏侧栏
 * （不持久化该派生状态，保证任何时刻打开结果一致）；自定义方案用持久化字节流
 * @param name 方案名
 * @return 应用成功返回 true
 */
bool DAAppLayoutManager::openLayout(const QString& name)
{
    if (name == QStringLiteral("Default")) {
        if (!mMainWindow->restoreUIState(mDefaultSnapshot)) {
            return false;
        }
        mMainWindow->showMaximized();
        return true;
    }
    if (name == QStringLiteral("Focus Analysis")) {
        if (!mMainWindow->restoreUIState(mDefaultSnapshot)) {
            return false;
        }
        // 默认布局基础上隐藏左右侧栏（D12 预置「专注分析」）
        if (DAAppDockingArea* dock = mMainWindow->getDockArea()) {
            dock->toggleLeftSidebar(false);
            dock->toggleRightSidebar(false);
        }
        mMainWindow->showMaximized();
        return true;
    }
    if (!mCustom.contains(name)) {
        daWarning << tr("Layout scheme '%1' not found").arg(name);  // cn:未找到布局方案"%1"
        return false;
    }
    if (!mMainWindow->restoreUIState(mCustom[ name ])) {
        daWarning << tr("Failed to restore layout scheme '%1', it may be saved by an incompatible version").arg(
            name);  // cn:布局方案"%1"恢复失败，可能由不兼容的版本保存
        return false;
    }
    mMainWindow->showMaximized();
    return true;
}

/**
 * @brief 删除自定义方案（预置不可删）
 */
bool DAAppLayoutManager::removeLayout(const QString& name)
{
    if (isPreset(name)) {
        daWarning << tr("Preset layout schemes cannot be removed");  // cn:预置布局方案不可删除
        return false;
    }
    if (!mCustom.contains(name)) {
        return false;
    }
    mCustom.remove(name);
    mCustomOrder.removeAll(name);
    saveCustomLayouts();
    return true;
}

/**
 * @brief 所有方案名（预置在前，自定义按保存顺序）
 */
QStringList DAAppLayoutManager::layoutNames() const
{
    QStringList names;
    names << QStringLiteral("Default") << QStringLiteral("Focus Analysis");
    names << mCustomOrder;
    return names;
}

/**
 * @brief 是否预置方案
 */
bool DAAppLayoutManager::isPreset(const QString& name) const
{
    return name == QStringLiteral("Default") || name == QStringLiteral("Focus Analysis");
}

/**
 * @brief 自定义方案持久化文件路径
 */
QString DAAppLayoutManager::layoutsFilePath() const
{
    return QDir::toNativeSeparators(QString("%1/dawork-layouts.json").arg(DAAbstractSettingPage::getConfigFileSavePath()));
}

/**
 * @brief 从磁盘加载自定义方案（方案名 → base64 字节流）
 */
void DAAppLayoutManager::loadCustomLayouts()
{
    QFile f(layoutsFilePath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        return;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (QJsonParseError::NoError != err.error || !doc.isObject()) {
        qWarning() << "DAAppLayoutManager: cannot parse" << layoutsFilePath() << err.errorString();
        return;
    }
    QJsonObject root = doc.object();
    mCustomOrder.clear();
    const QJsonArray order = root.value("order").toArray();
    for (const QJsonValue& v : order) {
        QString name = v.toString();
        if (name.isEmpty()) {
            continue;
        }
        QString b64 = root.value("layouts").toObject().value(name).toString();
        if (b64.isEmpty()) {
            continue;
        }
        mCustom[ name ] = QByteArray::fromBase64(b64.toUtf8());
        mCustomOrder.append(name);
    }
}

/**
 * @brief 写回自定义方案到磁盘
 */
void DAAppLayoutManager::saveCustomLayouts() const
{
    QJsonObject layouts;
    for (auto it = mCustom.constBegin(); it != mCustom.constEnd(); ++it) {
        layouts.insert(it.key(), QString::fromUtf8(it.value().toBase64()));
    }
    QJsonObject root;
    root.insert("order", QJsonArray::fromStringList(mCustomOrder));
    root.insert("layouts", layouts);
    QFile f(layoutsFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAppLayoutManager: cannot open" << layoutsFilePath() << "for writing";
        return;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

}  // namespace DA
