#include "DADataManagerTreeFilterProxyModel.h"
namespace DA
{
DADataManagerTreeFilterProxyModel::DADataManagerTreeFilterProxyModel(QObject* par) : DAStringFilterProxyModel(par)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterColumn(0);                // 只匹配名称列
    setIncludeParentsOfMatches(true);  // 数据系列命中时放行所属 dataframe
}

Qt::ItemFlags DADataManagerTreeFilterProxyModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    // 直接返回源模型的flags
    QModelIndex srcIndex = mapToSource(index);
    return sourceModel()->flags(srcIndex);
}

QMimeData* DADataManagerTreeFilterProxyModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.isEmpty())
        return nullptr;
    // 把索引映射回源，再让源模型生成 MIME
    QModelIndexList srcIndexes;
    for (const auto& idx : indexes) {
        srcIndexes << mapToSource(idx);
    }

    return sourceModel()->mimeData(srcIndexes);
}
}  // end DA
