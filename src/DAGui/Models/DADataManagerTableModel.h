#ifndef DADATAMANAGERTABLEMODEL_H
#define DADATAMANAGERTABLEMODEL_H
#include <QAbstractTableModel>
#include "DAGlobals.h"
#include "DADataManager.h"
#include "DAGuiAPI.h"
#ifndef DA_ROLE_DADATAMANAGERTABLEMODEL_DATA
/**
 * @def 获取数据的角色
 */
#define DA_ROLE_DADATAMANAGERTABLEMODEL_DATA Qt::UserRole + 1
#endif

namespace DA
{
/**
 * @brief 对DataManager显示的的model
 */
class DAGUI_API DADataManagerTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DADataManagerTableModel(QObject* p = nullptr);
    DADataManagerTableModel(DADataManager* dm, QObject* p = nullptr);
    ~DADataManagerTableModel();
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

public:
    void setDataManager(DADataManager* dm);
    QVariant dataDisplay(const DAData& d, const QModelIndex& index) const;
    QVariant dataDecoration(const DAData& d, const QModelIndex& index) const;
    QVariant dataBackground(const DAData& d, const QModelIndex& index) const;
    QVariant dataToolTip(const DAData& d, const QModelIndex& index) const;
    //变量对应的图标
    static QIcon dataToIcon(const DAData& d);

public:
    //刷新
    void refresh(int row, int col);
private slots:
    void onDataAdded(const DA::DAData& d);
    void onDataBeginRemoved(const DA::DAData& d, int dataIndex);
    void onDataRemoved(const DA::DAData& d, int dataIndex);

private:
    DADataManager* _dataManager;
};
}  // namespace DA
#endif  // DADATAMANAGERMODEL_H
