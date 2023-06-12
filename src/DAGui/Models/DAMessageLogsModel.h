#ifndef DAMESSAGELOGSMODEL_H
#define DAMESSAGELOGSMODEL_H
#include "DAGuiAPI.h"
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include "DAMessageQueueProxy.h"
#ifndef DA_ROLE_MESSAGE_TYPE
/**
 * @def  定义消息类型的角色，通过这个角色可以获取消息的类型
 */
#define DA_ROLE_MESSAGE_TYPE (Qt::UserRole + 1)
#endif

namespace DA
{
class DAMessageLogItem;
DA_IMPL_FORWARD_DECL(DAMessageLogsModel)
/**
 * @brief 用于显示全局消息的model
 */
class DAGUI_API DAMessageLogsModel : public QAbstractTableModel
{
    Q_OBJECT
    DA_IMPL(DAMessageLogsModel)
public:
    DAMessageLogsModel(QObject* p = nullptr);
    ~DAMessageLogsModel();

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    //获取内部维护的DAMessageQueueProxy
    const DAMessageQueueProxy& messageQueueProxy() const;
    DAMessageQueueProxy& messageQueueProxy();

public:
    //是否显示时间列
    void setShowDateTime(bool on = true);
    //是否显示时间
    bool isShowDateTime() const;
    //设置不同消息类型的背景颜色
    void setTypeBackgroundColor(QtMsgType type, const QColor& clr);
    //清空所有，此操作会把所有消息队列里的消息清空
    void clearAll();
private slots:
    //有消息插入触发的槽
    void onMessageAppended();
    void onMessageQueueSizeChanged(int newSize);

protected:
    virtual QVariant dataDisplay(DAMessageLogItem* item, const QModelIndex& index) const;
    virtual QVariant dataDecoration(DAMessageLogItem* item, const QModelIndex& index) const;
    virtual QVariant dataBackground(DAMessageLogItem* item, const QModelIndex& index) const;
    virtual QVariant dataToolTip(DAMessageLogItem* item, const QModelIndex& index) const;
};

/**
 * @brief 对message的SortFilterProxyModel，用于进行消息过滤
 */
class DAGUI_API DAMessageLogsSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    /**
     * @brief 定义接受的消息类型
     */
    enum AcceptMessageType
    {
        AcceptDebugMsg    = 0x01,  ///< 对应debug
        AcceptWarningMsg  = 0x02,  ///< 接受Warning
        AcceptCriticalMsg = 0x04,  ///< 接受Critical
        AcceptFatalMsg    = 0x08,  ///< 接受Fatal
        AcceptInfoMsg     = 0x10,  ///< 接受info
        AcceptAll         = 0x1F   ///< 接受所有（不过滤）
    };
    Q_ENUM(AcceptMessageType)
    Q_DECLARE_FLAGS(AcceptMessageTypeFlags, AcceptMessageType)
public:
    DAMessageLogsSortFilterProxyModel(QObject* p = nullptr);
    ~DAMessageLogsSortFilterProxyModel();
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

public:
    //设置flag
    void setAcceptMessageTypeFlag(AcceptMessageType t, bool on);
    //检测AcceptMessageType是否配置
    bool testAcceptMessageTypeFlag(AcceptMessageType t) const;
    //判断消息类型是否符合当前的过滤类型,如果符合，则接受这个消息
    bool isQtMsgTypeMatchAcceptType(int msgtype) const;

private:
    AcceptMessageTypeFlags _acceptsType;
};
}  // namespace DA
#endif  // DAMESSAGELOGSMODEL_H
