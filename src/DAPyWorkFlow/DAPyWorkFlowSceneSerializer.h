#ifndef DAPYWORKFLOWSCENESERIALIZER_H
#define DAPYWORKFLOWSCENESERIALIZER_H
#include "DAPyWorkFlowAPI.h"
#include <QDomDocument>
#include <QVersionNumber>

class QDomElement;

namespace DA
{
class DAPyWorkFlowScene;

/**
 * @brief Python工作流场景布局序列化器
 *
 * 专门负责DAPyWorkFlowScene的布局数据XML序列化和反序列化操作。
 * 仅保存/加载节点位置、图元可视化属性、连接线布局等场景级别数据，
 * 不处理Python工作流逻辑数据（节点拓扑、参数值、连接关系）。
 * Python工作流数据由DAPyWorkFlowSerializer序列化到独立的workflow-data.xml中。
 * 节点通过node_id与Python工作流数据关联。
 * 使用PIMPL模式隔离实现细节。
 *
 * @see DAPyWorkFlowScene DAPyWorkFlowSerializer
 */
class DAPYWORKFLOW_API DAPyWorkFlowSceneSerializer
{
    DA_DECLARE_PRIVATE(DAPyWorkFlowSceneSerializer)
public:
    DAPyWorkFlowSceneSerializer();
    ~DAPyWorkFlowSceneSerializer();

    // 保存场景到XML文档
    bool saveSceneToXml(const DAPyWorkFlowScene* scene,
                        QDomDocument* doc,
                        const QVersionNumber& ver = QVersionNumber(1, 0, 0));

    // 从XML元素加载场景
    bool loadSceneFromXml(const QDomElement* sceneElement,
                          DAPyWorkFlowScene* scene,
                          const QVersionNumber& ver = QVersionNumber(1, 0, 0));

    // 保存场景到XML文件
    bool saveSceneToFile(const DAPyWorkFlowScene* scene,
                         const QString& filePath,
                         const QVersionNumber& ver = QVersionNumber(1, 0, 0));

    // 从XML文件加载场景
    bool loadSceneFromFile(const QString& filePath,
                           DAPyWorkFlowScene* scene,
                           const QVersionNumber& ver = QVersionNumber(1, 0, 0));

    // 获取最后的错误信息
    QString getLastErrorString() const;
};

}  // namespace DA

#endif  // DAPYWORKFLOWSCENESERIALIZER_H