#ifndef DANODEDESCRIPTOR_H
#define DANODEDESCRIPTOR_H

#include "DAPyWorkFlowAPI.h"
#include "DAPortDescriptor.h"
#include "DAParameterDescriptor.h"
#include "DAPyNodeStyle.h"
#include "DAPyNodeStyleDefine.h"
#include "DAPyNodeFactory.h"
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>

namespace DA
{

/**
 * @brief 工作流节点描述符
 *
 * 轻量级数据结构，用于描述 Python 工作流节点的完整元数据，
 * 包含节点名称、分类、端口、参数、渲染模板及样式配置。
 * 支持与旧 Python dict 格式的 JSON 序列化/反序列化，
 * 以及转换为 DAPyNodeMetaData 用于节点工厂注册。
 *
 * 使用示例：
 * @code
 * DANodeDescriptor desc;
 * desc.name = "筛选节点";
 * desc.qualifiedName = "data_workbench.filter_node";
 * desc.category = "数据清洗";
 * desc.renderTemplate = RenderTemplate::NodeStyleTemplate;
 * QJsonObject json = desc.toJson();
 * DANodeDescriptor restored = DANodeDescriptor::fromJson(json);
 * @endcode
 *
 * @see DAPyNodeMetaData DAPortDescriptor ParameterDescriptor DANodeStyle
 */
struct DAPYWORKFLOW_API DANodeDescriptor
{
    QString name;                                 ///< 节点显示名称
    QString qualifiedName;                        ///< 节点唯一标识名（Python qualified_name）
    QString category;                             ///< 节点分组/分类
    QString icon;                                 ///< 节点图标路径
    QVector< DAPortDescriptor > inputs;           ///< 输入端口描述符列表
    QVector< DAPortDescriptor > outputs;          ///< 输出端口描述符列表
    QVector< DAParameterDescriptor > parameters;  ///< 参数描述符列表
    RenderTemplate renderTemplate;                ///< 渲染模板类型
    DANodeStyle style;                            ///< 节点样式配置

    // 默认构造函数
    DANodeDescriptor();

    // 判断描述符是否有效（qualifiedName 非空）
    bool isValid() const;

    // 转换为 DAPyNodeMetaData（提取注册所需字段）
    DAPyNodeMetaData toMetaData() const;

    // 序列化为 JSON（snake_case 键名，稀疏策略）
    QJsonObject toJson() const;

    // 从 JSON 反序列化（带默认值回退）
    static DANodeDescriptor fromJson(const QJsonObject& obj);
};

}  // namespace DA

#endif  // DANODEDESCRIPTOR_H
