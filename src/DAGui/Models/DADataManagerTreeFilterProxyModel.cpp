#include "DADataManagerTreeFilterProxyModel.h"
namespace DA
{
DADataManagerTreeFilterProxyModel::DADataManagerTreeFilterProxyModel(QObject* par) : QSortFilterProxyModel(par)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void DADataManagerTreeFilterProxyModel::setFilterText(const QString& text)
{
    m_filterText = text;
    invalidateFilter();
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


bool DADataManagerTreeFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (m_filterText.isEmpty()) {
        return true;
    }

    // 检查当前行是否匹配
    if (filterAcceptsRowItself(source_row, source_parent)) {
        return true;
    }

    // 检查子行是否匹配
    if (hasAcceptedChildren(source_row, source_parent)) {
        return true;
    }

    return false;
}

bool DADataManagerTreeFilterProxyModel::filterAcceptsRowItself(int source_row, const QModelIndex& source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    if (!index.isValid()) {
        return false;
    }

    QString text = sourceModel()->data(index, Qt::DisplayRole).toString();
    return text.contains(m_filterText, Qt::CaseInsensitive);
}

bool DADataManagerTreeFilterProxyModel::hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    if (!index.isValid()) {
        return false;
    }

    // 检查所有子行
    int childCount = sourceModel()->rowCount(index);
    for (int i = 0; i < childCount; ++i) {
        if (filterAcceptsRowItself(i, index) || hasAcceptedChildren(i, index)) {
            return true;
        }
    }

    return false;
}
}  // end DA
