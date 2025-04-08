#ifndef DADATAMANAGERINTERFACE_H
#define DADATAMANAGERINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DABaseInterface.h"
#include "DAData.h"
#include "DADataManager.h"
class QUndoStack;
namespace DA
{
class DACoreInterface;
/**
 * @brief 数据管理接口
 */
class DAINTERFACE_API DADataManagerInterface : public DABaseInterface
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DADataManagerInterface)
public:
	DADataManagerInterface(DACoreInterface* c, QObject* par = nullptr);
	~DADataManagerInterface();
	// 获取datamanager指针
	DADataManager* dataManager() const;
	// 添加数据
	virtual void addData(DAData& d);
	virtual void addData_(DAData& d);
	// 移除数据
	virtual void removeData(DAData& d);
	virtual void removeData_(DAData& d);
	// 获取数据量
	virtual int getDataCount() const;
	// 参数的索引
	int getDataIndex(const DAData& d) const;
	// 根据索引获取对应的值
	DAData getData(int index) const;
	// 根据id获取数据
	DAData getDataById(DAData::IdType id) const;
	// 获取undo stack
	QUndoStack* getUndoStack() const;
signals:
	/**
	 * @brief 有数据添加发射的信号
	 * @param d
	 */
	void dataAdded(const DA::DAData& d);
	/**
	 * @brief 数据准备删除
	 * @param d
	 * @param dataIndex
	 */
	void dataBeginRemove(const DA::DAData& d, int dataIndex);
	/**
	 * @brief 数据删除发射的信号
	 * @param d
	 */
	void dataRemoved(const DA::DAData& d, int dataOldIndex);
	/**
	 * @brief 数据信息改变
	 * @param d 数据
	 * @param oldname
	 */
	void dataChanged(const DA::DAData& d, DA::DADataManager::ChangeType t);
};
}  // namespace DA
#endif  // DADATAMANAGERINTERFACE_H
