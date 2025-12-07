#ifndef DADATAMANAGERTREEMODEL_H
#define DADATAMANAGERTREEMODEL_H
#include <functional>
#include <QObject>
#include <QStandardItemModel>
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataManager.h"
class QMimeData;

/**
 * @def 定义详细数据类型，目前只有在dataframe的series扩展用到
 */
#ifndef DADATAMANAGERTREEMODEL_ROLE_DETAIL_DATA_TYPE
#define DADATAMANAGERTREEMODEL_ROLE_DETAIL_DATA_TYPE (Qt::UserRole + 30)
#endif

namespace DA
{


/**
 * @brief 数据树模型，用于展示DADataManager中的数据
 */
class DAGUI_API DADataManagerTreeModel : public QStandardItemModel
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADataManagerTreeModel)

public:
    /**
     * @brief 特殊数据类型标记
     */
    enum DetailDataTypeMark
    {
        DataFrameItem        = 1,  ///< DataFrame数据项
        SeriesInnerDataframe = 2   ///< DataFrame内部的Series
    };

    /**
     * @brief 列类型
     */
    enum ColumnStyle
    {
        ColumnWithNameOnly = 1,  ///< 只有一列名字，默认
        ColumnWithNameProperty   ///< 名称和属性两列
    };

    /**
     * @brief 角色定义
     */
    enum CustomRole
    {
        RoleItemType       = Qt::UserRole + 10,  ///< 项目类型
        RoleDataId         = Qt::UserRole + 20,  ///< 数据ID
        RoleDetailDataType = Qt::UserRole + 30   ///< 详细数据类型
    };

public:
    DADataManagerTreeModel(QObject* parent = nullptr);
    DADataManagerTreeModel(DADataManager* dataMgr, QObject* parent = nullptr);
    DADataManagerTreeModel(DADataManager* dataMgr, ColumnStyle style, QObject* parent = nullptr);
    ~DADataManagerTreeModel() override;

    // 设置数据管理器
    void setDataManager(DADataManager* dataMgr);
    DADataManager* getDataManager() const;

    // DataFrame展开设置
    void setExpandDataframeToSeries(bool on);
    bool isExpandDataframeToSeries() const;

    // 列样式设置
    void setColumnStyle(ColumnStyle style);
    ColumnStyle getColumnStyle() const;

    // 数据查找
    QStandardItem* findItemByData(const DAData& data) const;
    QStandardItem* findItemByDataId(DAData::IdType id) const;

    // 数据转换
    DAData itemToData(QStandardItem* item) const;
    bool isDataframeItem(QStandardItem* item) const;
    bool isDataframeSeriesItem(QStandardItem* item) const;

    // 获取所有数据名称
    QStringList getAllDataNames() const;

    // 模型操作
    void clear();

    // 设置是否可编辑，可编辑则可以改变名字
    void setEnableEdit(bool on);
    bool isEnableEdit() const;

    // 重写基类方法
    virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

protected:
    // 创建数据项
    QStandardItem* createDataItem(const DAData& data);
    // 创建DataFrame的Series项
    QStandardItem* createDataFrameSeriesItem(const QString& seriesName, DAData::IdType dataframeId);

    // DataFrame展开处理
    void updateDataFrameExpansion();
    void updateDataFrameItemExpansion(QStandardItem* dataframeItem, bool expanded);
    // 创建tooltip
    QString makeDataToolTip(const DAData& data);

private:
    void initialize();
    void setupConnections();
    void rebuildDataMap();
    void clearDataMap();

    // 数据项管理
    void addDataItem(const DAData& data);
    void removeDataItem(const DAData& data);
    void updateDataItem(const DAData& data, DADataManager::ChangeType changeType);

private slots:
    void onDataAdded(const DAData& data);
    void onDataBeginRemoved(const DAData& data, int index);
    void onDataChanged(const DAData& data, DADataManager::ChangeType changeType);
    void onDatasCleared();
};

// 以下是递归函数用于遍历
bool standardItemIterator(QStandardItem* startItem,
                          std::function< bool(QStandardItem*, QStandardItem*) > fun,
                          bool firstColumnOnly = false);


}

#endif  // DADATAMANAGERTREEMODEL_H
