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
    // TODO 审查： 这里操作非常危险，如果删除节点没有告知DAAbstractNodeSettingWidget将会奔溃，考虑是否DAPyNodeProxy使用智能指针
    DA_D(d);
    d->mNodeProxy = proxy;
    if (proxy) {
        d->mDescriptor = proxy->getDescriptorStruct();
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
