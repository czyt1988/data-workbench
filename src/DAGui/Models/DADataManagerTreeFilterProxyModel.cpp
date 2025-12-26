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
