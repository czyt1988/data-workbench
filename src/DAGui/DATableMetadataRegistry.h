#ifndef DATABLEMETADATAREGISTRY_H
#define DATABLEMETADATAREGISTRY_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include <QObject>
#include <QHash>
#include <QList>
#include <QPair>

namespace DA
{
class DADataManager;
class DATableMetadata;

/**
 * @brief 表格元数据会话级注册表
 *
 * 按 DAData 键控管理 DATableMetadata（元数据束），使元数据生命周期脱离单个 table widget，
 * 随数据对象存在（关闭/重开窗口、工程加载均保留）。注册表由 DADataOperateWidget 拥有，
 * 会话级存活。束内子存储（样式、视图状态，后续含条件格式/批注/校验/计算列）由 DATableMetadata 聚合。
 *
 * 本类由 DATableStyleRegistry 泛化而来（T1.3 表格基础设施）：对外 getOrCreate/get/has/remove/clear/
 * nonEmptyEntries 语义与旧注册表一致，只是值类型从 DATableStyleManager* 变为 DATableMetadata*；
 * 取样式管理器的调用点改为 getOrCreate(d)->styleManager()。
 *
 * 生命周期约定：
 * - 数据移除（dataRemoved）时**保留**其元数据束，以支持撤销移除（DAData 身份不变仍命中）；
 * - datasCleared 信号时清空全部束；
 * - 永久移除的孤儿条目内存占用极小，保存时遍历 DADataManager.getAllDatas() 自然不写出孤儿。
 */
class DAGUI_API DATableMetadataRegistry : public QObject
{
    Q_OBJECT
public:
    explicit DATableMetadataRegistry(DADataManager* dm, QObject* parent = nullptr);
    ~DATableMetadataRegistry();

    // 获取或创建某数据的元数据束（束的 QObject parent = this）
    DATableMetadata* getOrCreate(const DAData& d);
    // 仅查询，不创建；不存在返回 nullptr
    DATableMetadata* get(const DAData& d) const;
    // 是否存在该数据的元数据束
    bool has(const DAData& d) const;
    // 显式移除并销毁某数据的束（一般由 datasCleared 统一处理，按需提供）
    void remove(const DAData& d);
    // 清空全部
    void clear();
    // 保存用：返回所有非空束及其 DAData（供 DAAppProject 遍历写出）
    QList< QPair< DAData, DATableMetadata* > > nonEmptyEntries() const;
Q_SIGNALS:
    // 新元数据束被创建时发射，widget 可借此连接信号
    void metadataCreated(const DA::DAData& d, DA::DATableMetadata* meta);

private:
    DADataManager* mDataManager;  ///< 非拥有，用于连接 datasCleared
    QHash< DAData, DATableMetadata* > mRegistry;
};
}  // end of namespace DA
#endif  // DATABLEMETADATAREGISTRY_H
