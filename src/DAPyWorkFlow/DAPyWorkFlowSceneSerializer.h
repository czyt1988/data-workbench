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
 * @brief Python工作流场景序列化器
 *
 * 专门负责DAPyWorkFlowScene的XML序列化和反序列化操作。
 * 保存场景中的节点位置、连接信息、节点参数等，加载时通过
 * DANodeRegistry重建节点图形项并恢复连接关系。
 * 使用PIMPL模式隔离实现细节。
 *
 * @code
 * DA::DAPyWorkFlowSceneSerializer serializer;
 * // 保存场景到XML文件
 * QDomDocument doc;
 * serializer.saveSceneToXml(&scene, &doc);
 * // 从XML文件加载场景
 * serializer.loadSceneFromXml(&doc.documentElement(), &scene);
 * @endcode
 *
 * @see DAPyWorkFlowScene DAWorkflowState
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