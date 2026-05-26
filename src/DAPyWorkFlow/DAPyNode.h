#ifndef DAPYNODE_H
#define DAPYNODE_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyObjectWrapper.h"
#include "DAPyNodeState.h"
#include "DAPyNodeStyleDefine.h"
#include "DAPyNodeStyle.h"
#include "DAPybind11InQt.h"
#include <QString>
#include <QList>

namespace DA
{

/**
 * @brief Python节点的C++纯代理类
 *
 * 代理Python NodeDef定义的节点，继承DAPyObjectWrapper，
 * 像DAPyDataFrame一样通过attr()与Python对象交互，
 * 不缓存任何Python数据到C++成员变量。
 *
 * 所有方法实时从Python对象读取属性，不做本地缓存。
 * 复合类型（端口、参数、样式）通过DictConverter从Python dict临时转换为C++ struct。
 *
 * @code
 * DAPyNode proxy(pyNodeObj);
 * if (!proxy.isNone()) {
 *     QString name = proxy.getNodeName();
 *     DANodeStyle style = proxy.getNodeStyle();
 * }
 * @endcode
 *
 * @see DAPyObjectWrapper DAPyDataFrame DAPyDictConverter
 */
class DAPYWORKFLOW_API DAPyNode : public DAPyObjectWrapper
{
public:
    // 构造/析构
    explicit DAPyNode();
    explicit DAPyNode(const pybind11::object& pyNode);
    explicit DAPyNode(pybind11::object&& pyNode);
    explicit DAPyNode(const DAPyObjectWrapper& pyNode);
    explicit DAPyNode(const DAPyNode& pyNode);
    ~DAPyNode();

    // 获取Python节点的node_id（从Python对象属性node_id读取）
    QString getNodeId() const;

    // Python限定名（从Python对象属性qualified_name读取）
    QString getQualifiedName() const;

    // 节点名称（从Python对象属性name读取）
    QString getNodeName() const;

    // 节点分组（从Python对象属性category读取）
    QString getNodeCategory() const;

    // 节点图标（从Python对象属性icon读取）
    QString getIcon() const;

    // 输入/输出端口key列表（从Python对象属性input_keys/output_keys读取）
    QList< QString > getInputKeys() const;
    QList< QString > getOutputKeys() const;

    // 节点样式（从Python对象属性_node_display.style dict读取，临时转换为C++ struct）
    DANodeStyle getNodeStyle() const;

    // 渲染模板（从Python对象属性_node_display.render_template str读取）
    RenderTemplate getRenderTemplate() const;

    // 节点执行状态（从Python对象属性_node_state读取）
    DAPyNodeState getNodeState() const;

    // Python原生数据传递
    void setPyInputData(const QString& key, const pybind11::object& data);
    pybind11::object getPyOutputData(const QString& key) const;

    // 配置数据传递（QJsonObject→Python dict→调用set_input_data）
    void setConfig(const QJsonObject& config);
};

}  // namespace DA

#endif  // DAPYNODE_H
