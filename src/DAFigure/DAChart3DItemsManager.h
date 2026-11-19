#ifndef DACHART3DITEMSMANAGER_H
#define DACHART3DITEMSMANAGER_H
#include "DAFigureAPI.h"
#include <QHash>
#include <QList>
#include <QString>

class Qwt3DPlotItem;

namespace DA
{

/**
 * @brief 三维图元（Qwt3DPlotItem）与存档 key 的对应关系管理者
 *
 * 二维用 @ref DAChartItemsManager 记录 QwtPlotItem，三维是另一套键空间（Qwt3DPlotItem
 * 的 rtti 从 1001 起、生命周期由业务层用 new/delete 管理），因此单独一个类：
 *
 * - 生命周期绑定一个 @ref DAChart3DWidget：图元 attach 时自动登记、detach（含删除）时注销，
 *   所以存档时不会读到悬空指针，也不会漏掉键；
 * - key 由 @ref makeItemKey 生成（`<rttiName>-<n>`）。计数器是**进程级**的，保证同一进程内
 *   不同 3D 子图、不同 figure 拿到的 key 互不重复——工程存档里全部 figure 的 3D 图元共用一个
 *   `chart3d-data/` 目录，key 就是文件名，跨 figure 重名会互相覆盖；
 * - 加载时同一个类可以当作「已读回图元的池子」用（@ref DAChart3DItemArchive::load 把读出的图元
 *   按存档里的 key 登记进来），figure XML 再按 key 取回并 attach。
 *
 * 本类不接管图元所有权：登记只是建立映射，删除图元仍是创建者/图表的事。
 */
class DAFIGURE_API DAChart3DItemsManager
{
public:
    DAChart3DItemsManager();
    ~DAChart3DItemsManager();

    // 登记图元并生成 key（已登记则返回原 key，不重复分配）
    QString recordItem(Qwt3DPlotItem* item);
    // 用指定 key 登记（加载存档时按存档里的 key 登记，保证 XML 里的 key 能查到）
    void recordItem(Qwt3DPlotItem* item, const QString& key);
    // 注销图元的登记（detach/删除时调用，双向映射一起清）
    void unrecordItem(Qwt3DPlotItem* item);
    // 图元对应的 key，未登记返回空
    QString itemToKey(Qwt3DPlotItem* item) const;
    // key 对应的图元，未登记返回 nullptr
    Qwt3DPlotItem* keyToItem(const QString& key) const;
    // 生成一个唯一的 key（不登记）
    QString makeItemKey(Qwt3DPlotItem* item);
    // 是否没有任何登记
    bool isEmpty() const;
    // 登记数量
    int size() const;
    // 获取所有 key
    QList< QString > keys() const;
    // 获取所有图元
    QList< Qwt3DPlotItem* > items() const;
    // 清空登记（不删除图元）
    void clear();
    // 3D 图元 rtti 的名字（Surface/Bar/Line/Waterfall/Item），用于 key 前缀
    static QString rttiName(int rtti);

private:
    QHash< Qwt3DPlotItem*, QString > mItemToKey;
    QHash< QString, Qwt3DPlotItem* > mKeyToItem;
};
}  // namespace DA

#endif  // DACHART3DITEMSMANAGER_H