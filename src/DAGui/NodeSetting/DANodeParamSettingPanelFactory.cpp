#include "DANodeParamSettingPanelFactory.h"
// DANodeParamSettingPanel 已在头文件中通过 #include "DANodeParamSettingPanel.h" 引入

namespace DA
{

/**
 * @brief 获取工厂单例实例
 */
DANodeParamSettingPanelFactory& DANodeParamSettingPanelFactory::instance()
{
    static DANodeParamSettingPanelFactory sInstance;
    return sInstance;
}

/**
 * @brief 注册面板创建函数
 * @param qualifiedName 节点的 qualifiedName
 * @param creator 面板创建函数
 */
void DANodeParamSettingPanelFactory::registerPanel(const QString& qualifiedName, FpCreatePanel creator)
{
    if (creator) {
        mCreators[qualifiedName] = creator;
    }
}

/**
 * @brief 根据 qualifiedName 创建对应面板
 * @param qualifiedName 节点的 qualifiedName
 * @param parent 父控件指针
 * @return 创建的面板实例，失败返回 nullptr
 */
DANodeParamSettingPanel* DANodeParamSettingPanelFactory::createPanel(const QString& qualifiedName, QWidget* parent) const
{
    auto it = mCreators.constFind(qualifiedName);
    if (it != mCreators.constEnd() && it.value()) {
        return it.value()(parent);
    }
    return nullptr;
}

/**
 * @brief 检查 qualifiedName 是否已注册
 * @param qualifiedName 节点的 qualifiedName
 * @return 已注册返回 true
 */
bool DANodeParamSettingPanelFactory::hasPanel(const QString& qualifiedName) const
{
    return mCreators.contains(qualifiedName);
}

/**
 * @brief 获取所有已注册的 qualifiedName 列表
 * @return 已注册的 qualifiedName 列表
 */
QStringList DANodeParamSettingPanelFactory::registeredNames() const
{
    return mCreators.keys();
}

/**
 * @brief 注册默认面板（占位方法）
 *
 * 当前为空实现，将在后续任务中注册通用面板。
 */
void DANodeParamSettingPanelFactory::registerDefaultPanels()
{
    // TODO(Task 8): 注册通用面板
    // registerPanel("generic", [](QWidget* parent) { return new DANodeParamSettingPanel(parent); });
}

}  // end namespace DA
