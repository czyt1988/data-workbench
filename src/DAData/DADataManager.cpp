#include "DADataManager.h"
#include <QList>
#include <QMap>
#include <QDebug>
#include <QUndoStack>
// DAUtils
#include "DAStringUtil.h"
//
#include "DACommandsDataManager.h"
namespace DA
{

class DADataManager::PrivateData
{
    DA_DECLARE_PUBLIC(DADataManager)
public:
    PrivateData(DADataManager* p);
    QList< DAData > _dataList;
    QMap< DAData::IdType, DAData > _dataMap;
    bool _dirtyFlag;               ///< 标记是否dirty
    QUndoStack _dataManagerStack;  ///< 数据管理的stack
};

//===================================================
// DADataManagerPrivate
//===================================================

DADataManager::PrivateData::PrivateData(DADataManager* p) : q_ptr(p), _dirtyFlag(false)
{
}

//===================================================
// DADataManager
//===================================================
DADataManager::DADataManager(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
}

DADataManager::~DADataManager()
{
}

/**
 * @brief 添加数据
 *
 * @note 此函数会发生信号@sa dataAdded
 * @param d
 */
void DADataManager::addData(DAData& d)
{
    if (d_ptr->_dataMap.contains(d.id())) {
        // 说明已经添加过
        qWarning() << tr("data:%1 have been added").arg(d.getName());
        if (d.getDataManager() != this) {
            // 说明这个data引用没有获取到datamanager
            d.setDataManager(this);
        }
        return;
    }
    setUniqueDataName(d);
    d.setDataManager(this);
    d_ptr->_dataList.push_back(d);
    d_ptr->_dataMap[ d.id() ] = d;
    setDirtyFlag(true);
    emit dataAdded(d);
}
/**
 * @brief 带redo/undo的添加数据
 *
 * @note 此函数会发生信号@sa dataBeginAdd 和 @sa dataEndAdded
 * @param d
 */
void DADataManager::addData_(DAData& d)
{
    DACommandDataManagerAdd* cmd = new DACommandDataManagerAdd(d, this);
    d_ptr->_dataManagerStack.push(cmd);
}

/**
 * @brief 添加数据
 * @param d
 * @return 返回数据DAData
 */
DAData DADataManager::addData(const DAAbstractData::Pointer& d)
{
    DAData res(d);
    addData(res);
    return res;
}
/**
 * @brief 带redo/undo的添加数据
 * @param d
 * @return 返回数据DAData
 */
DAData DADataManager::addData_(const DAAbstractData::Pointer& d)
{
    DAData res(d);
    addData_(res);
    return res;
}
/**
 * @brief 批量添加数据
 *
 * 等同多次调用addData_，会发射多次信号
 * @param d
 */
void DADataManager::addDatas_(const QList< DAData >& datas)
{
    std::unique_ptr< QUndoCommand > cmdGroup(new QUndoCommand(tr("add datas")));
    for (const DAData& d : datas) {
        new DACommandDataManagerAdd(d, this, cmdGroup.get());
    }
    if (cmdGroup->childCount() > 0) {
        d_ptr->_dataManagerStack.push(cmdGroup.release());
    }
}

/**
 * @brief 移除数据
 *
 * @note 此函数会发生信号@sa dataRemoved
 * @param d
 */
void DADataManager::removeData(DAData& d)
{
    int index = d_ptr->_dataList.indexOf(d);
    emit dataBeginRemove(d, index);
    d_ptr->_dataList.removeAt(index);
    d_ptr->_dataMap.remove(d.id());
    d.setDataManager(nullptr);
    setDirtyFlag(true);
    emit dataRemoved(d, index);
}

/**
 * @brief 带redo/undo的移除数据
 * @note 此函数会发生信号@sa dataBeginRemove 和 @sa dataEndRemoved
 * @param d
 */
void DADataManager::removeData_(DAData& d)
{
    DACommandDataManagerRemove* cmd = new DACommandDataManagerRemove(d, this);
    d_ptr->_dataManagerStack.push(cmd);
}

/**
 * @brief 批量移除数据
 * @param datas
 */
void DADataManager::removeDatas_(const QList< DAData >& datas)
{
    std::unique_ptr< QUndoCommand > cmdGroup(new QUndoCommand(tr("remove datas")));
    for (const DAData& d : datas) {
        new DACommandDataManagerRemove(d, this, cmdGroup.get());
    }
    if (cmdGroup->childCount() > 0) {
        d_ptr->_dataManagerStack.push(cmdGroup.release());
    }
}

/**
 * @brief 获取变量管理器管理的数据数量
 * @return
 */
int DADataManager::getDataCount() const
{
    return d_ptr->_dataList.size();
}

/**
 * @brief 参数在变量管理器的索引
 *
 * 参数在变量管理器中有一个list来维护，这个索引就是链表的索引
 * @param d
 * @return 如果没有在管理器中找到data，返回-1
 */
int DADataManager::getDataIndex(const DAData& d) const
{
    return d_ptr->_dataList.indexOf(d);
}

/**
 * @brief 根据索引获取对应的值
 * @param index
 * @return
 */
DAData DADataManager::getData(int index) const
{
    return d_ptr->_dataList.at(index);
}

/**
 * @brief 根据id获取数据
 * @param id
 * @return
 */
DAData DADataManager::getDataById(DAData::IdType id) const
{
    return d_ptr->_dataMap.value(id, DAData());
}

/**
 * @brief 判断是否dirty，数据的改变和添加都会把此flag标记为true
 * @return
 */
bool DADataManager::isDirty() const
{
    return d_ptr->_dirtyFlag;
}

/**
 * @brief 设置脏标记
 *
 * 此函数在 判断 是否 需要保存时使用
 * @param on
 */
void DADataManager::setDirtyFlag(bool on)
{
    d_ptr->_dirtyFlag = on;
}

/**
 * @brief 获取undo stack
 * @return
 */
QUndoStack* DADataManager::getUndoStack() const
{
    return &(d_ptr->_dataManagerStack);
}

/**
 * @brief 触发DataChanged信号
 * @param d
 * @param t
 */
void DADataManager::callDataChangedSignal(const DAData& d, DADataManager::ChangeType t)
{
    setDirtyFlag(true);
    emit dataChanged(d, t);
}

void DADataManager::setUniqueDataName(DAData& d) const
{
    QString n = d.getName();
    if (n.isEmpty()) {
        n = d.typeToString();
        d.setName(n);
    }
    QSet< QString > names = getDatasNameSet();
    // 构造一个唯一的名字
    n = DA::makeUniqueString(names, n);
    d.setName(n);
}
/**
 * @brief 把所有管理的变量的名字按照set返回
 * @return
 */
QSet< QString > DADataManager::getDatasNameSet() const
{
    QSet< QString > names;
    for (const DAData& d : qAsConst(d_ptr->_dataList)) {
        names.insert(d.getName());
    }
    return names;
}

}
