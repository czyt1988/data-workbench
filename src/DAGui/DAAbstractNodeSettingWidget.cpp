#include "DAAbstractNodeSettingWidget.h"
#include <QJsonArray>

namespace DA
{

class DAAbstractNodeSettingWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAAbstractNodeSettingWidget)

public:
    explicit PrivateData(DAAbstractNodeSettingWidget* q) : q_ptr(q)
    {
    }

    DAPyNodeProxy* mNodeProxy = nullptr;
    DANodeDescriptor mDescriptor;
};

// ============================================================
// 构造与析构
// ============================================================

/**
 * @brief 构造函数
 *
 * 初始化 PIMPL 私有数据。
 *
 * @param[in] parent 父窗口指针
 */
DAAbstractNodeSettingWidget::DAAbstractNodeSettingWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数
 */
DAAbstractNodeSettingWidget::~DAAbstractNodeSettingWidget()
{
}

// ============================================================
// 节点代理管理
// ============================================================

/**
 * @brief 设置节点代理
 *
 * 使用 QPointer 安全持有 DAPyNodeProxy，并在设置时缓存描述符。
 * 传入 nullptr 时清除缓存。
 *
 * @param[in] proxy 节点代理指针，可为 nullptr
 */
void DAAbstractNodeSettingWidget::setNodeProxy(DAPyNodeProxy* proxy)
{
    // ⚠️ 生命周期风险：此原始指针归 DAPyNodeGraphicsItem 所有（通过 unique_ptr 管理）。
    // 如果节点被删除（Delete 键/Undo/clearScene），此指针将悬空，后续访问会导致崩溃。
    // 改进方案（后续）：
    //   1. 使用 scene->findNodeItemByProxy(proxy) 在使用前验证指针有效性
    //   2. 在 scene 销毁节点时通过信号通知此面板清空指针
    //   3. 考虑使用观察者模式或 weak_ptr 替代原始指针
    DA_D(d);
    d->mNodeProxy = proxy;
    if (proxy) {
        d->mDescriptor.name           = proxy->getNodeName();
        d->mDescriptor.qualifiedName  = proxy->getQualifiedName();
        d->mDescriptor.category       = proxy->getNodeGroup();
        d->mDescriptor.icon           = proxy->getIcon();
        d->mDescriptor.inputs         = proxy->getInputPorts();
        d->mDescriptor.outputs        = proxy->getOutputPorts();
        d->mDescriptor.parameters     = proxy->getParameters();
        d->mDescriptor.renderTemplate = proxy->getRenderTemplate();
        d->mDescriptor.style          = proxy->getNodeStyle();
    }
}

/**
 * @brief 获取节点代理
 *
 * @return 当前持有的节点代理指针（可能为 nullptr）
 */
DAPyNodeProxy* DAAbstractNodeSettingWidget::getNodeProxy() const
{
    DA_DC(d);
    return d->mNodeProxy;
}

// ============================================================
// 描述符访问
// ============================================================

/**
 * @brief 获取缓存的节点描述符
 *
 * @return 缓存的 QJsonObject 描述符，未设置时返回空对象
 */
const DANodeDescriptor& DAAbstractNodeSettingWidget::getDescriptor() const
{
    return d_ptr->mDescriptor;
}

/**
 * @brief 从描述符中提取 parameters 数组
 *
 * @return 描述符中的 "parameters" 字段，不存在时返回空 QJsonArray
 */
const QVector< DAParameterDescriptor >& DAAbstractNodeSettingWidget::getParameters() const
{
    return d_ptr->mDescriptor.parameters;
}

}  // namespace DA
