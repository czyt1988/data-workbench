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
 * @brief 针对DAData的dataframe的item
 */
class DAGUI_API DAStandardItemDataDataframe : public QStandardItem
{
public:
    enum
    {
        Type = QStandardItem::UserType + 100  // 任意 >= UserType 的值
    };
    int type() const override
    {
        return Type;
    }
    explicit DAStandardItemDataDataframe(const DAData& data);
    ~DAStandardItemDataDataframe();
    QVariant data(int role = Qt::UserRole + 1) const override;
    //    virtual void setData(const QVariant& value, int role = Qt::UserRole + 1) override;
    // 获取dataframe
    DAData getDataframe() const;
    //
    static bool isDataframeItem(QStandardItem* item);

private:
    DAData m_dataframe;
};

/**
 * @brief 针对DAData的dataframe下的series的item
 */
class DAGUI_API DAStandardItemDataDataframeSeries : public QStandardItem
{
public:
    enum
    {
        Type = QStandardItem::UserType + 101  // 任意 >= UserType 的值
    };
    int type() const override
    {
        return Type;
    }
    explicit DAStandardItemDataDataframeSeries(const DAData& data, const QString& seriesName);
    ~DAStandardItemDataDataframeSeries();
    QVariant data(int role = Qt::UserRole + 1) const override;
    // 获取dataframe
    DAData getDataframe() const;
    // 获取series
    DAPySeries getSeries() const;
    // series的名字
    void setSeriesName(const QString& name);
    QString getSeriesName() const;
    // 判断是否是series
    static bool isDataframeSeriesItem(QStandardItem* item);
    // 获取描述文字
    static QString makeDescribeText(const DAData& data, const QString& seriesName);
    // 获取图标
    static QIcon seriesTypeToIcon(const DAData& data, const QString& seriesName);

private:
    DAData m_dataframe;
    QString m_name;
};

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
        ColumnWithNameProperty   ///< 名称和属性两列（TODO）
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
    QStandardItem* findFirstDataframeItemByData(const DAData& data) const;

    // 数据转换
    static DAData itemToData(QStandardItem* item);

    // 获取所有数据名称
    QStringList getAllDataframeNames() const;

    // 模型操作
    void clear();

    // 重写基类方法
    /**
    virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    **/

    // drag
    virtual Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    virtual QMimeData* mimeData(const QModelIndexList& indexs) const override;

    //
    static bool isDataframeItem(QStandardItem* item);
    static bool isDataframeSeriesItem(QStandardItem* item);
    // 创建数据项
    static QStandardItem* createDataItem(const DAData& data);
    // 创建DataFrame的Series项
    static QStandardItem* createDataFrameSeriesItem(const QString& seriesName, const DAData& dataframeData);

protected:
    // DataFrame展开处理
    void updateDataFrameExpansion();
    void updateDataFrameItemExpansion(QStandardItem* dataframeItem, bool expanded);
    // 创建tooltip

private:
    void initialize();
    void setupConnections();

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
