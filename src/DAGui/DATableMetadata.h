#ifndef DATABLEMETADATA_H
#define DATABLEMETADATA_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
class DATableStyleManager;
class DATableViewStateStore;

/**
 * @brief 一个数据集的表元数据束
 *
 * 会话级注册表 DATableMetadataRegistry 按 DAData 键控持有一份本对象，聚合该数据集
 * 的所有表级元数据子存储。v1 只含两个子存储：
 * - styleManager：行/列/单元格静态样式与列显示格式（既有能力）
 * - viewState：视图状态（列宽按列名 / 行头宽，R6）
 *
 * 条件格式 / 批注 / 校验 / 计算列由后续计划（003/004/005）各自往束里加子存储与
 * XML 段，本类只提供聚合与统一序列化入口 toXml/fromXml。
 *
 * 归属分层（D3）：渲染类子存储（样式，后续含条件格式/批注）与语义类子存储
 * （视图状态按列名引用；后续校验/计算列的定义类放 src/DAData）都挂在本束下，
 * 存储与序列化统一走注册表 → 工程文件 table-metadata.xml。
 *
 * 生命周期：本对象与其子存储的 QObject parent 均为注册表；随数据存在，widget 关闭不销毁。
 */
class DAGUI_API DATableMetadata : public QObject
{
    Q_OBJECT
public:
    explicit DATableMetadata(QObject* parent = nullptr);
    ~DATableMetadata();

    // 样式管理器（随束创建，parent = this，非空）
    DATableStyleManager* styleManager() const;
    // 视图状态子存储（随束创建，parent = this，非空）
    DATableViewStateStore* viewState() const;

    // 聚合序列化：把各子存储的 XML 段写入 table 元素
    void toXml(QDomDocument& doc, QDomElement& tableEle) const;
    // 聚合反序列化：从 table 元素读取各子存储的 XML 段（旧 table-styles.xml 无 <view-state> 段，自动跳过）
    bool fromXml(const QDomElement& tableEle);
    // 全部子存储均为空返回 true（保存时跳过空条目）
    bool isEmpty() const;

private:
    DATableStyleManager* mStyleManager { nullptr };    ///< 样式子存储（拥有，parent=this）
    DATableViewStateStore* mViewState { nullptr };     ///< 视图状态子存储（拥有，parent=this）
};
}  // end of namespace DA
#endif  // DATABLEMETADATA_H
