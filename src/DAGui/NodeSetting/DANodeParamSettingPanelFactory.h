#ifndef DANODEPARAMSETTINGPANELFACTORY_H
#define DANODEPARAMSETTINGPANELFACTORY_H
#include "DAGuiAPI.h"
#include "DANodeParamSettingPanel.h"
#include <functional>
#include <QHash>
#include <QString>
#include <QStringList>

class QWidget;

namespace DA
{

// DANodeParamSettingPanel 已通过上方头文件完整引入

/**
 * @brief 节点参数设置面板工厂类（单例）
 *
 * 通过 qualifiedName 映射到对应的设置面板创建函数，消除 switch-case 路由。
 * 插件可通过 registerPanel() 扩展自定义节点参数面板。
 *
 * @see DANodeParamSettingPanel
 *
 * @code
 * // 获取所有已注册的 qualifiedName
 * QStringList names = DANodeParamSettingPanelFactory::instance().registeredNames();
 *
 * // 根据 qualifiedName 创建面板
 * DANodeParamSettingPanel* panel = DANodeParamSettingPanelFactory::instance().createPanel("pandas.read_csv", parentWidget);
 *
 * // 注册自定义面板
 * DANodeParamSettingPanelFactory::instance().registerPanel("MyNode", [](QWidget* parent){ return new MyCustomPanel(parent); });
 * @endcode
 *
 * @since 2026-05-04
 */
class DAGUI_API DANodeParamSettingPanelFactory
{
public:
    using FpCreatePanel = std::function<DANodeParamSettingPanel*(QWidget* parent)>;

    // 获取工厂单例实例
    static DANodeParamSettingPanelFactory& instance();

    // 注册面板创建函数
    void registerPanel(const QString& qualifiedName, FpCreatePanel creator);

    // 根据 qualifiedName 创建对应面板
    DANodeParamSettingPanel* createPanel(const QString& qualifiedName, QWidget* parent = nullptr) const;

    // 检查 qualifiedName 是否已注册
    bool hasPanel(const QString& qualifiedName) const;

    // 获取所有已注册的 qualifiedName 列表
    QStringList registeredNames() const;

    // 注册默认面板（占位方法）
    void registerDefaultPanels();

private:
    // 构造函数私有化，单例模式
    DANodeParamSettingPanelFactory() = default;
    ~DANodeParamSettingPanelFactory() = default;
    // 禁止拷贝和赋值
    DANodeParamSettingPanelFactory(const DANodeParamSettingPanelFactory&) = delete;
    DANodeParamSettingPanelFactory& operator=(const DANodeParamSettingPanelFactory&) = delete;

private:
    QHash<QString, FpCreatePanel> mCreators; ///< qualifiedName → 创建函数映射
};

}  // end namespace DA

#endif  // DANODEPARAMSETTINGPANELFACTORY_H
