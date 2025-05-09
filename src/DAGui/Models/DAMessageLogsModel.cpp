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
	DAMessageQueueProxy _messageQueueProxy;
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
	connect(&(d_ptr->_messageQueueProxy),
            &DAMessageQueueProxy::messageQueueSizeChanged,
            this,
            &DAMessageLogsModel::onMessageQueueSizeChanged);
	connect(&(d_ptr->_messageQueueProxy), &DAMessageQueueProxy::messageQueueAppended, this, &DAMessageLogsModel::onMessageAppended);
	d_ptr->_rowCount = d_ptr->_messageQueueProxy.size();
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
				return tr("date time");
			case 1:
				return tr("message");
			default:
				return QVariant();
			}
		} else {
			return tr("message");
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
	DAMessageLogItem item = d_ptr->_messageQueueProxy.at(index.row());
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
 * @brief 获取内部维护的DAMessageQueueProxy
 * @return
 */
const DAMessageQueueProxy& DAMessageLogsModel::messageQueueProxy() const
{
    return d_ptr->_messageQueueProxy;
}

/**
 * @brief 获取内部维护的DAMessageQueueProxy
 * @return
 */
DAMessageQueueProxy& DAMessageLogsModel::messageQueueProxy()
{
    return d_ptr->_messageQueueProxy;
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
    d_ptr->_messageQueueProxy.clear();
}

void DAMessageLogsModel::onMessageAppended()
{
	// 全表刷新
	// 触发此信号说明队列已经满了
	int r = rowCount() - 1;
	int c = columnCount() - 1;
	int qs = d_ptr->_messageQueueProxy.size() - 1;  // 全局队列的容积，如果r == s 说明已经充满，不需要新增行

	// 防止清空队列后出现负索引
	r = (r > 0) ? r : 0;  // 禁止这种操作，会让r在索引和数量上处于一种模糊，在r有值时是索引，无值时也应该是索引只是是-1而已，如果是0，就认为有一个数据，会出现异常
	c  = (c > 0) ? c : 0;
	qs = (qs > 0) ? qs : 0;

	if (r < qs) {
		// 说明刚刚过容积线，此时需要插入到qs的长度，理论上之后都是r == qs
		// 此时r已经是减去1的索引，因此插入位置要r+1
		if (r != 0) {
			// r为空不需要处理,一般是刚启动的时候的情况
			// qDebug() << "beginInsertRows(QModelIndex()," << r + 1 << "," << qs << ")";
			beginInsertRows(QModelIndex(), r + 1, qs);
			d_ptr->_rowCount = qs + 1;
			endInsertRows();
		}
	} else {
		// 这里说明总体容积已经充满，全局队列此时会一直维护一个固定容积，只需要更新数据
		// 全局队列是进行一个移动，这里全部更新
		//  r和c已经在数量上减去1，就是索引
		if (r >= 0 && c >= 0) {
			// qDebug() << "emit dataChanged(index(0, 0), index(" << r << "," << c << ")";
			emit dataChanged(index(0, 0), index(r, c));
		}
	}
}

void DAMessageLogsModel::onMessageQueueSizeChanged(int newSize)
{
	// 全表刷新
	Q_UNUSED(newSize);
	int r = rowCount() - 1;
	int s = d_ptr->_messageQueueProxy.size() - 1;
    // qDebug() << "onMessageQueueSizeChanged:" << newSize;
	// 防止清空队列后出现负索引
	s = (s > 0) ? s : 0;
	r = (r > 0) ? r : 0;

	if (r < s) {
		beginInsertRows(QModelIndex(), r, s);
		d_ptr->_rowCount = s + 1;
		endInsertRows();
	} else if (s < r) {
		// 一般是进行了clear操作导致队列的尺寸变小
		beginRemoveRows(QModelIndex(), s, r);
		d_ptr->_rowCount = s + 1;
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
    : QSortFilterProxyModel(p), _acceptsType(AcceptAll)
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
	_acceptsType.setFlag(t, on);
	invalidateFilter();
}

/**
 * @brief 检测AcceptMessageType是否配置
 * @param t
 * @return
 */
bool DAMessageLogsSortFilterProxyModel::testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptMessageType t) const
{
    return _acceptsType.testFlag(t);
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
		return _acceptsType.testFlag(AcceptDebugMsg);
	case QtWarningMsg:
		return _acceptsType.testFlag(AcceptWarningMsg);
	case QtCriticalMsg:
		return _acceptsType.testFlag(AcceptCriticalMsg);
	case QtFatalMsg:
		return _acceptsType.testFlag(AcceptFatalMsg);
	case QtInfoMsg:
		return _acceptsType.testFlag(AcceptInfoMsg);
	default:
		break;
	}
	return false;
}
