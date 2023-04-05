#include "DADataManagerInterface.h"
namespace DA
{
class DADataManagerInterfacePrivate
{
    DA_IMPL_PUBLIC(DADataManagerInterface)
public:
    DADataManagerInterfacePrivate(DADataManagerInterface* p);
    DADataManager* _dataMgr;
};
//==============================================================
// DADataManagerInterfacePrivate
//==============================================================
DADataManagerInterfacePrivate::DADataManagerInterfacePrivate(DADataManagerInterface* p) : q_ptr(p)
{
    _dataMgr = new DADataManager(p);
}

//==============================================================
// DADataManagerInterface
//==============================================================
DADataManagerInterface::DADataManagerInterface(DACoreInterface* c, QObject* par)
    : DABaseInterface(c, par), d_ptr(new DADataManagerInterfacePrivate(this))
{

    connect(d_ptr->_dataMgr, &DADataManager::dataAdded, this, &DADataManagerInterface::dataAdded);
    connect(d_ptr->_dataMgr, &DADataManager::dataBeginRemove, this, &DADataManagerInterface::dataBeginRemove);
    connect(d_ptr->_dataMgr, &DADataManager::dataRemoved, this, &DADataManagerInterface::dataRemoved);
    connect(d_ptr->_dataMgr, &DADataManager::dataChanged, this, &DADataManagerInterface::dataChanged);
}

DADataManagerInterface::~DADataManagerInterface()
{
}
/**
 * @brief 获取数据管理的指针
 * @return
 */
DADataManager* DADataManagerInterface::dataManager() const
{
    return d_ptr->_dataMgr;
}
/**
 * @brief 添加数据
 *
 * @note 此函数会发生信号@sa dataAdded
 * @param d
 */
void DADataManagerInterface::addData(DAData& d)
{
    dataManager()->addData(d);
}
/**
 * @brief 移除数据
 *
 * @note 此函数会发生信号@sa dataRemoved
 * @param d
 */
void DADataManagerInterface::removeData(DAData& d)
{
    dataManager()->removeData(d);
}
/**
 * @brief 获取变量管理器管理的数据数量
 * @return
 */
int DADataManagerInterface::getDataCount() const
{
    return dataManager()->getDataCount();
}
/**
 * @brief 参数在变量管理器的索引
 *
 * 参数在变量管理器中有一个list来维护，这个索引就是链表的索引
 * @param d
 * @return
 */
int DADataManagerInterface::getDataIndex(const DAData& d) const
{
    return dataManager()->getDataIndex(d);
}
/**
 * @brief 根据索引获取对应的值
 * @param index
 * @return
 */
DAData DADataManagerInterface::getData(int index) const
{
    return dataManager()->getData(index);
}
/**
 * @brief 根据id获取数据
 * @param id
 * @return
 */
DAData DADataManagerInterface::getDataById(DAData::IdType id) const
{
    return dataManager()->getDataById(id);
}

}
