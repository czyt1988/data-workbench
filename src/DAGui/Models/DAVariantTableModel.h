#ifndef DAVARIANTTABLEMODEL_H
#define DAVARIANTTABLEMODEL_H
#include "DAGuiAPI.h"
#include "DATable.h"
#include <QAbstractTableModel>

namespace DA
{
/**
 * @brief 对DATable<QVariant>显示的的model
 */
class DAGUI_API DAVariantTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DAVariantTableModel(QObject* p = nullptr);
    DAVariantTableModel(DATable< QVariant >* d, QObject* p = nullptr);
    ~DAVariantTableModel();
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    DATable< QVariant >* mData { nullptr };
};
}

#endif  // DAVARIANTTABLEMODEL_H
