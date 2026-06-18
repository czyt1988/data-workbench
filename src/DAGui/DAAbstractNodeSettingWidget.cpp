#include "DAAbstractNodeSettingWidget.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPyBindQt/DAPybind11QtCaster.hpp"
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

    DAPyNode mNodeProxy;
    DAPyNodeMetaData mMetaData;
    QList< DAPyNodeParameter > mParamDefs;
};

// ============================================================
// 构造与析构
// ============================================================

DAAbstractNodeSettingWidget::DAAbstractNodeSettingWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
}

DAAbstractNodeSettingWidget::~DAAbstractNodeSettingWidget()
{
}

// ============================================================
// 节点代理管理
// ============================================================

/**
 * @brief 设置节点代理
 *
 * 按值持有 DAPyNode（内部引用计数 pybind11::object），并在设置时缓存元数据和参数代理列表。
 * 传入 isNone() 代理时清除缓存。
 *
 * @param[in] proxy 节点代理常量引用
 */
void DAAbstractNodeSettingWidget::setNode(const DAPyNode& proxy)
{
    DA_D(d);
    d->mNodeProxy = proxy;
    d->mParamDefs.clear();
    if (!proxy.isNone()) {
        // 缓存元数据
        d->mMetaData.name          = proxy.getNodeName();
        d->mMetaData.qualifiedName = proxy.getQualifiedName();
        d->mMetaData.category      = proxy.getNodeCategory();
        d->mMetaData.iconPath      = proxy.getIcon();

        // 通过DAPyNode::getParameters()获取参数代理列表
        d->mParamDefs = proxy.getParameters();
    }
}

const DAPyNode& DAAbstractNodeSettingWidget::getNode() const
{
    DA_DC(d);
    return d->mNodeProxy;
}

DAPyNode& DAAbstractNodeSettingWidget::node()
{
    DA_D(d);
    return d->mNodeProxy;
}

// ============================================================
// 元数据访问
// ============================================================

const DAPyNodeMetaData& DAAbstractNodeSettingWidget::getMetaData() const
{
    DA_DC(d);
    return d->mMetaData;
}

const QList< DAPyNodeParameter >& DAAbstractNodeSettingWidget::getParamDefs() const
{
    DA_DC(d);
    return d->mParamDefs;
}

}  // namespace DA
