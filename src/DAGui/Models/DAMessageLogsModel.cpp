#include "DAMessageLogsModel.h"
#include "DAMessageLogItem.h"
#include <QDebug>
#include <QIcon>

namespace DA
{
class DAMessageLogsModelPrivate
{
	DA_IMPL_PUBLIC(DAMessageLogsModel)
public:
	DAMessageLogsModelPrivate(DAMessageLogsModel* p);

public:
	bool _showDateTime;
	int _rowCount;
	QColor _bgClrDebug;
	QColor _bgClrWarning;
	QColor _bgClrCritical;
	QColor _bgClrInfo;
};

}  // namespace DA
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAMessageLogsModelPrivate
//===================================================
DAMessageLogsModelPrivate::DAMessageLogsModelPrivate(DAMessageLogsModel* p)
    : q_ptr(p)
    , _showDateTime(true)
    , _rowCount(0)
    , _bgClrDebug(0, 0, 255, 40)
    , _bgClrWarning(255, 252, 0, 40)
    , _bgClrCritical(255, 0, 0, 40)
{
}
//===================================================
// DAMessageLogsModel
//===================================================
DAMessageLogsModel::DAMessageLogsModel(QObject* p) : QAbstractTableModel(p), d_ptr(new DAMessageLogsModelPrivate(this))
{
	auto& queue = DAMessageLogQueue::instance();
	connect(&queue, &DAMessageLogQueue::messageQueueSizeChanged,
            this, &DAMessageLogsModel::onMessageQueueSizeChanged);
	connect(&queue, &DAMessageLogQueue::messageQueueAppended, this, &DAMessageLogsModel::onMessageAppended);
	d_ptr->_rowCount = queue.size();
}

DAMessageLogsModel::~DAMessageLogsModel()
{
}

QVariant DAMessageLogsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();
	if (Qt::Horizontal == orientation) {  // 说明是水平表头
		if (isShowDateTime()) {
			switch (section) {
			case 0:
				return tr("date time");  // cn:日期时间
			case 1:
				return tr("message");  // cn:消息
			default:
				return QVariant();
			}
		} else {
			return tr("message");  // cn:消息
		}
	} else {
		return section + 1;
	}
	return QVariant();
}

int DAMessageLogsModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return d_ptr->_showDateTime ? 2 : 1;
}

int DAMessageLogsModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return d_ptr->_rowCount;
}

QVariant DAMessageLogsModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (index.row() >= d_ptr->_rowCount) {
		return QVariant();
	}
	DAMessageLogItem item = DAMessageLogQueue::instance().at(index.row());
	switch (role) {
	case Qt::TextAlignmentRole:
		return int(Qt::AlignLeft | Qt::AlignVCenter);
	case Qt::DisplayRole:
		return dataDisplay(&item, index);
	case Qt::DecorationRole:
		return dataDecoration(&item, index);
	case Qt::BackgroundRole:
		return dataBackground(&item, index);
	case Qt::ToolTipRole:
		return dataToolTip(&item, index);
	case DA_ROLE_MESSAGE_TYPE:  // 返回消息的类型
		return (int)item.getMsgType();
	default:
		break;
	}

	return QVariant();
}

Qt::ItemFlags DAMessageLogsModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool DAMessageLogsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	Q_UNUSED(index);
	Q_UNUSED(value);
	Q_UNUSED(role);
	return false;
}

/**
 * @brief 设置显示时间列
 * @param on
 */
void DAMessageLogsModel::setShowDateTime(bool on)
{
    d_ptr->_showDateTime = on;
}

/**
 * @brief 是否显示时间
 * @return
 */
bool DAMessageLogsModel::isShowDateTime() const
{
    return d_ptr->_showDateTime;
}

/**
 * @brief 设置消息类型的背景颜色
 * @param type
 * @param clr
 */
void DAMessageLogsModel::setTypeBackgroundColor(QtMsgType type, const QColor& clr)
{
	switch (type) {
	case QtDebugMsg:
		d_ptr->_bgClrDebug = clr;
		break;
	case QtWarningMsg:
		d_ptr->_bgClrWarning = clr;
		break;
	case QtCriticalMsg:
	case QtFatalMsg:
		d_ptr->_bgClrCritical = clr;
		break;
	case QtInfoMsg:
		d_ptr->_bgClrInfo = clr;
		break;
	default:
		break;
	}
}

/**
 * @brief 清空所有，此操作会把所有消息队列里的消息清空
 */
void DAMessageLogsModel::clearAll()
{
    DAMessageLogQueue::instance().clear();
    if (d_ptr->_rowCount > 0) {
        beginRemoveRows(QModelIndex(), 0, d_ptr->_rowCount - 1);
        d_ptr->_rowCount = 0;
        endRemoveRows();
    }
}

void DAMessageLogsModel::onMessageAppended()
{
	// 触发此信号说明队列已满（惰性信号合并了多次插入）
	int qs = DAMessageLogQueue::instance().size();
	if (d_ptr->_rowCount < qs) {
		// 模型行数少于队列尺寸，需要插入新行
		beginInsertRows(QModelIndex(), d_ptr->_rowCount, qs - 1);
		d_ptr->_rowCount = qs;
		endInsertRows();
	} else if (d_ptr->_rowCount > 0) {
		// 行数已对齐，队列在循环覆写，只需刷新数据
		int lastRow = d_ptr->_rowCount - 1;
		int lastCol = columnCount() - 1;
		emit dataChanged(index(0, 0), index(lastRow, lastCol));
	}
}

void DAMessageLogsModel::onMessageQueueSizeChanged(int newSize)
{
	if (newSize > d_ptr->_rowCount) {
		// 队列增长，插入新行
		beginInsertRows(QModelIndex(), d_ptr->_rowCount, newSize - 1);
		d_ptr->_rowCount = newSize;
		endInsertRows();
	} else if (newSize < d_ptr->_rowCount) {
		// 队列缩小（一般是 clear 操作），移除多余行
		beginRemoveRows(QModelIndex(), newSize, d_ptr->_rowCount - 1);
		d_ptr->_rowCount = newSize;
		endRemoveRows();
	}
}

QVariant DAMessageLogsModel::dataDisplay(DAMessageLogItem* item, const QModelIndex& index) const
{
	switch (index.column()) {
	case 0:
		return isShowDateTime() ? item->datetimeToString() : item->getMsg();
	case 1:
		return isShowDateTime() ? item->getMsg() : QVariant();
	default:
		break;
	}
	return QVariant();
}

QVariant DAMessageLogsModel::dataDecoration(DAMessageLogItem* item, const QModelIndex& index) const
{
	static QIcon s_iconMessageTypeDebug = QIcon(":/DAGui/MessageType/icon/messageType/messageTypeDebug.svg");
	static QIcon s_iconMessageTypeInfo  = QIcon(":/DAGui/MessageType/icon/messageType/messageTypeInfo.svg");
	static QIcon s_iconMessageTypeWarn  = QIcon(":/DAGui/MessageType/icon/messageType/messageTypeWarning.svg");
	static QIcon s_iconMessageTypeError = QIcon(":/DAGui/MessageType/icon/messageType/messageTypeError.svg");
	if (0 != index.column()) {
		return QVariant();
	}
	switch (item->getMsgType()) {
	case QtDebugMsg:
		return s_iconMessageTypeDebug;
	case QtWarningMsg:
		return s_iconMessageTypeWarn;
	case QtCriticalMsg:
	case QtFatalMsg:
		return s_iconMessageTypeError;
	case QtInfoMsg:
	default:
		return s_iconMessageTypeInfo;
	}
	return QVariant();
}

QVariant DAMessageLogsModel::dataBackground(DAMessageLogItem* item, const QModelIndex& index) const
{
	Q_UNUSED(index);
	switch (item->getMsgType()) {
	case QtDebugMsg:
		return d_ptr->_bgClrDebug.isValid() ? d_ptr->_bgClrDebug : QVariant();
	case QtWarningMsg:
		return d_ptr->_bgClrWarning.isValid() ? d_ptr->_bgClrWarning : QVariant();
	case QtCriticalMsg:
	case QtFatalMsg:
		return d_ptr->_bgClrCritical.isValid() ? d_ptr->_bgClrCritical : QVariant();
	case QtInfoMsg:
		return d_ptr->_bgClrInfo.isValid() ? d_ptr->_bgClrInfo : QVariant();
	default:
		break;
	}
	return QVariant();
}

QVariant DAMessageLogsModel::dataToolTip(DAMessageLogItem* item, const QModelIndex& index) const
{
	switch (index.column()) {
	case 0:
		return isShowDateTime() ? item->datetimeToString() : item->getMsg();
	case 1:
		return isShowDateTime() ? item->getMsg() : QVariant();
	default:
		break;
	}
	return QVariant();
}

//===================================================
// DAMessageLogsSortFilterProxyModel
//===================================================

DAMessageLogsSortFilterProxyModel::DAMessageLogsSortFilterProxyModel(QObject* p)
    : QSortFilterProxyModel(p), mAcceptsType(AcceptAll)
{
}

DAMessageLogsSortFilterProxyModel::~DAMessageLogsSortFilterProxyModel()
{
}

bool DAMessageLogsSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	bool isok          = false;
	QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
	int msgtype        = sourceModel()->data(index0, DA_ROLE_MESSAGE_TYPE).toInt(&isok);
	return isQtMsgTypeMatchAcceptType(msgtype);
}

/**
 * @brief 设置可接受的消息类型
 * @param t
 * @param on
 */
void DAMessageLogsSortFilterProxyModel::setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptMessageType t,
                                                                 bool on)
{
	mAcceptsType.setFlag(t, on);
	invalidateFilter();
}

/**
 * @brief 检测AcceptMessageType是否配置
 * @param t
 * @return
 */
bool DAMessageLogsSortFilterProxyModel::testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptMessageType t) const
{
    return mAcceptsType.testFlag(t);
}

/**
 * @brief 判断消息类型是否符合当前的过滤类型,如果符合，则接受这个消息
 * @param msgtype QtMsgType类型：QtDebugMsg，QtWarningMsg，QtCriticalMsg，QtFatalMsg，QtInfoMsg
 * @return
 */
bool DAMessageLogsSortFilterProxyModel::isQtMsgTypeMatchAcceptType(int msgtype) const
{
	switch (msgtype) {
	case QtDebugMsg:
		return mAcceptsType.testFlag(AcceptDebugMsg);
	case QtWarningMsg:
		return mAcceptsType.testFlag(AcceptWarningMsg);
	case QtCriticalMsg:
		return mAcceptsType.testFlag(AcceptCriticalMsg);
	case QtFatalMsg:
		return mAcceptsType.testFlag(AcceptFatalMsg);
	case QtInfoMsg:
		return mAcceptsType.testFlag(AcceptInfoMsg);
	default:
		break;
	}
	return false;
}
