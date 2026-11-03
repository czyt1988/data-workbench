#ifndef DANODEPARAMETERFORMADAPTER_H
#define DANODEPARAMETERFORMADAPTER_H

#include "DAGuiAPI.h"
#include "DAFormSpec.h"          // from DAUtils
#include "DAPyWorkFlow/DAPyNodeParameter.h"
#include <QList>
#include <QString>

namespace DA
{
/**
 * @brief 节点参数代理到通用表单规格的适配器
 *
 * 将 DAPyNodeParameter 列表转换为 DAFormSpec，供 DAPropertyFormWidget 渲染。
 * 替代原 DAParamDef + DAParamTypeRegistry 的参数定义层，统一走 DAFormEditorRegistry
 * 的编辑器创建/读取/写入/信号连接机制。
 *
 * 类型字符串归一化逻辑镜像自原 DAParamDef::normalizeTypeName：
 * string→str、boolean→bool、double→float、integer→int，其余原样小写化。
 *
 * @see DAFormSpec DAPropertyFormWidget DAPyNodeParameter
 */
class DAGUI_API DANodeParameterFormAdapter
{
public:
    // 将节点参数列表转换为表单规格；title 非空时写入 spec.title
    static DAFormSpec toFormSpec(const QList< DAPyNodeParameter >& parameters, const QString& title = QString());
};
}  // namespace DA

#endif  // DANODEPARAMETERFORMADAPTER_H
