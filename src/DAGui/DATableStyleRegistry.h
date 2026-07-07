#ifndef DATABLESTYLEREGISTRY_H
#define DATABLESTYLEREGISTRY_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include <QObject>
#include <QHash>
#include <QList>
#include <QPair>

namespace DA
{
class DADataManager;
class DATableStyleManager;

/**
 * @brief 表格样式会话级注册表
 *
 * 按 DAData 键控管理 DATableStyleManager，使样式生命周期脱离单个 table widget，
 * 随数据对象存在（关闭/重开窗口、工程加载均保留）。注册表由 DADataOperateWidget 拥有，
 * 会话级存活。样式类本身仍在 DAGui 模块（非下沉 DAData），此处仅做"逻辑随数据"的会话级缓存。
 *
 * 生命周期约定：
 * - 数据移除（dataRemoved）时**保留**其 manager，以支持撤销移除（DAData 身份不变仍命中）；
 * - datasCleared 信号时清空全部 manager；
 * - 永久移除的孤儿条目内存占用极小，保存时遍历 DADataManager.getAllDatas() 自然不写出孤儿。
 */
class DAGUI_API DATableStyleRegistry : public QObject
{
    Q_OBJECT
public:
    explicit DATableStyleRegistry(DADataManager* dm, QObject* parent = nullptr);
    ~DATableStyleRegistry();

    // 获取或创建某数据的样式管理器（manager 的 QObject parent = this）
    DATableStyleManager* getOrCreate(const DAData& d);
    // 仅查询，不创建；不存在返回 nullptr
    DATableStyleManager* get(const DAData& d) const;
    // 是否存在该数据的样式管理器
    bool has(const DAData& d) const;
    // 显式移除并销毁某数据的 manager（一般由 datasCleared 统一处理，按需提供）
    void remove(const DAData& d);
    // 清空全部
    void clear();
    // 保存用：返回所有非空 manager 及其 DAData（供 DAAppProject 遍历写出）
    QList< QPair< DAData, DATableStyleManager* > > nonEmptyEntries() const;
Q_SIGNALS:
    // 新管理器被创建时发射，widget 可借此连接信号（目前 widget 在构造时一次性连接，未用此信号）
    void styleManagerCreated(const DA::DAData& d, DA::DATableStyleManager* mgr);

private:
    DADataManager* mDataManager;  ///< 非拥有，用于连接 datasCleared
    QHash< DAData, DATableStyleManager* > mRegistry;
};
}  // end of namespace DA
#endif  // DATABLESTYLEREGISTRY_H
