#include "DANodeParamSettingPanelWidget.h"
#include "DANodeParamSettingPanel.h"
#include "DANodeParamSettingPanelFactory.h"
#include "DAPyNodeProxy.h"
#include <QStackedWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QJsonObject>

namespace DA
{

class DANodeParamSettingPanelWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DANodeParamSettingPanelWidget)

public:
    explicit PrivateData(DANodeParamSettingPanelWidget* q)
        : q_ptr(q)
    {
    }

    QStackedWidget* mStackedWidget                = nullptr;
    QHash<QString, DANodeParamSettingPanel*> mPanelCache;
    DANodeParamSettingPanel* mCurrentPanel        = nullptr;
    QLabel* mPlaceholderLabel                     = nullptr;
    DAPyNodeProxy* mNodeProxy                     = nullptr;
};

/**
 * @brief 构造函数
 *
 * 创建 QStackedWidget 布局，添加占位标签 "未选中节点" 作为默认页面。
 * @param parent 父控件
 */
DANodeParamSettingPanelWidget::DANodeParamSettingPanelWidget(QWidget* parent)
    : QWidget(parent)
    , DA_PIMPL_CONSTRUCT
{
    DA_D(d);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    d->mStackedWidget = new QStackedWidget(this);
    layout->addWidget(d->mStackedWidget);

    // 创建占位标签
    d->mPlaceholderLabel = new QLabel(QStringLiteral("未选中节点"), d->mStackedWidget);
    d->mPlaceholderLabel->setObjectName(QStringLiteral("da_node_placeholder_label"));
    d->mPlaceholderLabel->setAlignment(Qt::AlignCenter);
    d->mPlaceholderLabel->setEnabled(false);
    d->mStackedWidget->addWidget(d->mPlaceholderLabel);
}

/**
 * @brief 析构函数
 *
 * 清除所有缓存的面板（qDeleteAll），面板作为 QStackedWidget 的子控件
 * 会在父控件销毁时自动清理，但 clearCache 确保缓存映射也被清除。
 */
DANodeParamSettingPanelWidget::~DANodeParamSettingPanelWidget()
{
    DA_D(d);
    qDeleteAll(d->mPanelCache);
    d->mPanelCache.clear();
}

/**
 * @brief 设置节点代理 — 主入口
 *
 * 核心调度逻辑：
 * 1. 如果代理为空 → 切换到占位标签，mCurrentPanel = nullptr
 * 2. 如果代理有效 → 从代理获取描述符 → 提取 qualifiedName
 * 3. 在缓存中查找面板 → 未命中则通过工厂惰性创建
 * 4. 工厂创建失败 → 使用 buildDefaultPanel() 创建默认面板
 * 5. 添加面板到 QStackedWidget → 缓存 → 连接信号 → 切换
 * 6. 调用面板的 setNodeProxy(proxy) 和 updateUI()
 *
 * @param proxy 节点代理指针，nullptr 表示取消选中
 */
void DANodeParamSettingPanelWidget::setNodeProxy(DAPyNodeProxy* proxy)
{
    DA_D(d);

    // 断开当前面板的信号连接
    if (d->mCurrentPanel) {
        disconnect(d->mCurrentPanel, &DANodeParamSettingPanel::propertyValueChanged,
                   this, &DANodeParamSettingPanelWidget::propertyValueChanged);
    }

    // 代理为空 → 显示占位标签
    if (nullptr == proxy) {
        d->mCurrentPanel = nullptr;
        d->mNodeProxy    = nullptr;
        d->mStackedWidget->setCurrentWidget(d->mPlaceholderLabel);
        return;
    }

    d->mNodeProxy = proxy;

    // 从代理获取描述符，提取 qualifiedName
    const DANodeDescriptor& descriptor = proxy->getDescriptorStruct();
    QString qualifiedName              = descriptor.qualifiedName;
    if (qualifiedName.isEmpty()) {
        qualifiedName = QStringLiteral("generic");
    }

    // 在缓存中查找
    DANodeParamSettingPanel* panel = nullptr;
    if (d->mPanelCache.contains(qualifiedName)) {
        panel = d->mPanelCache[qualifiedName];
    }

    // 缓存未命中 → 惰性创建
    if (nullptr == panel) {
        panel = DANodeParamSettingPanelFactory::instance().createPanel(qualifiedName, this);
        // 工厂无法创建 → 使用默认面板
        if (nullptr == panel) {
            panel = buildDefaultPanel();
        }
        if (panel) {
            d->mPanelCache[qualifiedName] = panel;
            d->mStackedWidget->addWidget(panel);
        }
    }

    if (nullptr == panel) {
        // 仍然无法创建面板 → 显示占位标签
        d->mCurrentPanel = nullptr;
        d->mStackedWidget->setCurrentWidget(d->mPlaceholderLabel);
        return;
    }

    // 连接面板信号 → 转发至外部
    connect(panel, &DANodeParamSettingPanel::propertyValueChanged,
            this, &DANodeParamSettingPanelWidget::propertyValueChanged);

    // 切换到对应面板
    d->mCurrentPanel = panel;
    d->mStackedWidget->setCurrentWidget(panel);

    // 设置代理并更新 UI
    panel->setNodeProxy(proxy);
    panel->updateUI();
}

/**
 * @brief 清除所有缓存的面板
 *
 * 从 QStackedWidget 中移除所有缓存面板并删除，
 * 切换回占位标签，清空当前面板指针和缓存映射。
 */
void DANodeParamSettingPanelWidget::clearCache()
{
    DA_D(d);

    // 断开当前面板的信号
    if (d->mCurrentPanel) {
        disconnect(d->mCurrentPanel, &DANodeParamSettingPanel::propertyValueChanged,
                   this, &DANodeParamSettingPanelWidget::propertyValueChanged);
    }

    // 从 QStackedWidget 中移除并删除缓存的面板
    for (auto it = d->mPanelCache.begin(); it != d->mPanelCache.end(); ++it) {
        DANodeParamSettingPanel* panel = it.value();
        if (panel) {
            d->mStackedWidget->removeWidget(panel);
            panel->deleteLater();
        }
    }
    d->mPanelCache.clear();
    d->mCurrentPanel = nullptr;

    // 切换回占位标签
    d->mStackedWidget->setCurrentWidget(d->mPlaceholderLabel);
}

/**
 * @brief 获取当前显示的面板
 * @return 当前面板指针，若无面板则返回 nullptr
 */
DANodeParamSettingPanel* DANodeParamSettingPanelWidget::currentPanel() const
{
    DA_DC(d);
    return d->mCurrentPanel;
}

/**
 * @brief 工厂无法创建面板时的默认面板创建方法
 *
 * 创建一个普通的 DANodeParamSettingPanel 作为兜底面板。
 * @return 默认面板实例
 */
DANodeParamSettingPanel* DANodeParamSettingPanelWidget::buildDefaultPanel()
{
    return new DANodeParamSettingPanel(this);
}

/**
 * @brief 使用模拟描述符测试调度逻辑（测试辅助方法）
 *
 * 绕过 DAPyNodeProxy，直接使用模拟的 QJsonObject 描述符测试
 * 面板创建、缓存和切换逻辑。从描述符中提取 qualifiedName，
 * 调用工厂创建面板（或使用默认面板），添加到 QStackedWidget 并缓存。
 * 不调用面板的 setNodeProxy 和 updateUI（因无真实代理）。
 *
 * @param descriptor 模拟的节点描述符 QJsonObject
 */
void DANodeParamSettingPanelWidget::testSetNodeProxyWithDescriptor(const QJsonObject& descriptor)
{
    DA_D(d);

    // 断开当前面板的信号连接
    if (d->mCurrentPanel) {
        disconnect(d->mCurrentPanel, &DANodeParamSettingPanel::propertyValueChanged,
                   this, &DANodeParamSettingPanelWidget::propertyValueChanged);
    }

    // 从描述符提取 qualifiedName
    QString qualifiedName = descriptor["qualified_name"].toString();
    if (qualifiedName.isEmpty()) {
        qualifiedName = QStringLiteral("generic");
    }

    // 在缓存中查找
    DANodeParamSettingPanel* panel = nullptr;
    if (d->mPanelCache.contains(qualifiedName)) {
        panel = d->mPanelCache[qualifiedName];
    }

    // 缓存未命中 → 惰性创建
    if (nullptr == panel) {
        panel = DANodeParamSettingPanelFactory::instance().createPanel(qualifiedName, this);
        // 工厂无法创建 → 使用默认面板
        if (nullptr == panel) {
            panel = buildDefaultPanel();
        }
        if (panel) {
            d->mPanelCache[qualifiedName] = panel;
            d->mStackedWidget->addWidget(panel);
        }
    }

    if (nullptr == panel) {
        d->mCurrentPanel = nullptr;
        d->mStackedWidget->setCurrentWidget(d->mPlaceholderLabel);
        return;
    }

    // 连接面板信号 → 转发至外部
    connect(panel, &DANodeParamSettingPanel::propertyValueChanged,
            this, &DANodeParamSettingPanelWidget::propertyValueChanged);

    // 切换到对应面板（不调用 setNodeProxy/updateUI，因无真实代理）
    d->mCurrentPanel = panel;
    d->mStackedWidget->setCurrentWidget(panel);
}

}  // namespace DA