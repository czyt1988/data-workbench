#ifndef DADATAMANAGERTREEFILTERPROXYMODEL_H
#define DADATAMANAGERTREEFILTERPROXYMODEL_H
#include <QSortFilterProxyModel>
#include "DAGuiAPI.h"
namespace DA
{
/**
 * @brief 代理模型用于过滤
 */
class DAGUI_API DADataManagerTreeFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    DADataManagerTreeFilterProxyModel(QObject* par = nullptr);
    // 设置过滤文本
    void setFilterText(const QString& text);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    bool filterAcceptsRowItself(int source_row, const QModelIndex& source_parent) const;
    bool hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const;

private:
    QString m_filterText;
};
}  // end DA
#endif  // DADATAMANAGERTREEFILTERPROXYMODEL_H
