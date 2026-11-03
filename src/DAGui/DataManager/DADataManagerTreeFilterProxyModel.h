#ifndef DADATAMANAGERTREEFILTERPROXYMODEL_H
#define DADATAMANAGERTREEFILTERPROXYMODEL_H
#include "DAGuiAPI.h"
#include "DAStringFilterProxyModel.h"
namespace DA
{
/**
 * @brief 数据管理器树的过滤代理
 *
 * 通用过滤逻辑（大小写、列、父节点放行）来自 DAStringFilterProxyModel；
 * 本类只保留数据管理器特有的 flags/mimeData 映射（编辑位与拖拽 MIME 都要穿透代理）。
 */
class DAGUI_API DADataManagerTreeFilterProxyModel : public DAStringFilterProxyModel
{
    Q_OBJECT
public:
    DADataManagerTreeFilterProxyModel(QObject* par = nullptr);
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
};
}  // end DA
#endif  // DADATAMANAGERTREEFILTERPROXYMODEL_H
