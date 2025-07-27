#include "DARecentFilesManager.h"
#include <QCoreApplication>
#include <QSettings>
#include <QActionGroup>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QFont>

namespace DA
{

/**
 * @brief 构造一个 DARecentFilesManager 实例
 * @param parent       父对象
 * @param maxEntries   列表最大条目数
 * @param orgName      组织名（用于 QSettings），留空则用 QCoreApplication::organizationName()
 * @param appName      应用名（用于 QSettings），留空则用 QCoreApplication::applicationName()
 */
DARecentFilesManager::DARecentFilesManager(QObject* parent, int maxEntries, const QString& orgName, const QString& appName)
    : QObject(parent), m_max(maxEntries)
{
    m_org = orgName.isEmpty() ? QCoreApplication::organizationName() : orgName;
    m_app = appName.isEmpty() ? QCoreApplication::applicationName() : appName;

    m_group = new QActionGroup(this);
    connect(m_group, &QActionGroup::triggered, this, [ this ](QAction* a) {
        QString path = a->data().toString();
        if (fileExists(path)) {
            Q_EMIT fileSelected(path);
        } else {
            m_files.removeAll(path);
            writeSettings();
            rebuildMenu();
        }
    });

    readAndCleanSettings();
}

/**
 * @brief 将“最近文件”子菜单挂到指定父菜单
 * @param parentMenu 父级菜单
 * @param title      子菜单标题
 */
void DARecentFilesManager::attachToMenu(QMenu* parentMenu, const QString& title)
{
    if (!m_recentMenu) {
        m_recentMenu = parentMenu->addMenu(title);
        rebuildMenu();
    }
}

/**
 * @brief 向列表新增一条记录
 * @param filePath 文件绝对路径
 *
 * 如果该路径已存在，会被提到最前面；超出最大条目数时末尾会被删除。
 */
void DARecentFilesManager::addFile(const QString& filePath)
{
    const QString abs = QFileInfo(filePath).absoluteFilePath();
    m_files.removeAll(abs);
    m_files.prepend(abs);
    while (m_files.size() > m_max) {
        m_files.removeLast();
    }
    writeSettings();
    if (m_recentMenu) {
        rebuildMenu();
    }
}

/**
 * @brief 清空所有记录
 */
void DARecentFilesManager::clear()
{
    m_files.clear();
    writeSettings();
    if (m_recentMenu)
        rebuildMenu();
}

/**
 * @brief 重新扫描：删除不存在的文件
 *
 * 扫描完成后如有变化，会立即保存并刷新菜单。
 */
void DARecentFilesManager::rescan()
{
    bool changed = false;
    for (auto it = m_files.begin(); it != m_files.end();) {
        if (!fileExists(*it)) {
            it      = m_files.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    if (changed) {
        writeSettings();
        rebuildMenu();
    }
}

/**
 * @brief 根据当前 m_files 重建菜单
 *
 * 不存在文件对应的 QAction 会被设置为带删除线字体。
 */
void DARecentFilesManager::rebuildMenu()
{
    if (!m_recentMenu)
        return;

    m_recentMenu->clear();
    if (m_files.isEmpty()) {
        QAction* none = m_recentMenu->addAction(tr("(empty)"));  // cn:空
        none->setEnabled(false);
        return;
    }

    for (const QString& f : m_files) {
        QAction* act = m_recentMenu->addAction(f);
        act->setData(f);
        act->setToolTip(f);

        if (!fileExists(f)) {
            QFont font = act->font();
            font.setStrikeOut(true);
            act->setFont(font);
        }
        m_group->addAction(act);
    }
    m_recentMenu->addSeparator();
    QAction* clearAct = m_recentMenu->addAction(tr("Clear menu"));  // cn:清空
    connect(clearAct, &QAction::triggered, this, &DARecentFilesManager::clear);
}

/**
 * @brief 首次读取配置文件并剔除不存在文件
 *
 * 读取 QSettings 中的 "RecentFiles" 键值后，立即过滤掉无效路径。
 * 若列表发生变化，会写回设置。
 */
void DARecentFilesManager::readAndCleanSettings()
{
    QSettings s(m_org, m_app);
    m_files = s.value("RecentFiles").toStringList();

    bool changed = false;
    for (auto it = m_files.begin(); it != m_files.end();) {
        if (!fileExists(*it)) {
            it      = m_files.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    if (changed) {
        writeSettings();
    }
}

/**
 * @brief 将当前列表写入持久化设置
 */
void DARecentFilesManager::writeSettings()
{
    QSettings s(m_org, m_app);
    s.setValue("RecentFiles", m_files);
}

/**
 * @brief 判断给定路径的文件是否存在
 * @param filePath 文件绝对路径
 * @return true 存在，false 不存在
 */
bool DARecentFilesManager::fileExists(const QString& filePath) const
{
    return QFileInfo::exists(filePath);
}
}  // end DA
