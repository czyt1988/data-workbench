#ifndef DADATAMANAGER_H
#define DADATAMANAGER_H
#include <QObject>
#include "DADataAPI.h"
#include "DAData.h"
class QUndoStack;
namespace DA
{
/**
 * @brief DAData的数据管理类，实现数据操作的一些通知例如数据添加、删除、改名、内容改变等等
 *
 * 通过此类可以有效的管理DAData数据
 *
 * @note 注意，后缀带_的方法是支持redo/undo的，如removeData是不带redo/undo，removeData_是带redo/undo
 */
class DADATA_API DADataManager : public QObject
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DADataManager)
	friend class DAData;

public:
	/**
	 * @brief 改变类型
	 */
	enum ChangeType
	{
		ChangeName,                ///< 改变了名字
		ChangeDescribe,            ///< 改变了描述
		ChangeValue,               ///< 改变了值
		ChangeDataframeColumnName  ///< 针对dataframe，改变了列名
	};
	Q_ENUM(ChangeType)
public:
	DADataManager(QObject* par = nullptr);
	~DADataManager();
	// 添加数据,返回数据的名字，添加数据时数据名字有可能会改变，数据的重复添加并不会带来影响
	void addData(DAData& d);
	void addData_(DAData& d);
	DAData addData(const DAAbstractData::Pointer& d);
	DAData addData_(const DAAbstractData::Pointer& d);
	// 添加多个
	void addDatas_(const QList< DAData >& datas);
	// 移除数据
	void removeData(DAData& d);
	void removeData_(DAData& d);
	void removeDatas_(const QList< DAData >& datas);
	// 获取数据量
	int getDataCount() const;
	// 参数的索引
	int getDataIndex(const DAData& d) const;
	// 根据索引获取对应的值
	DAData getData(int index) const;
	// 根据id获取数据
	DAData getDataById(DAData::IdType id) const;
	// 判断是否dirty，数据的改变和添加都会把此flag标记为true
	bool isDirty() const;
	// 设置脏标记
	void setDirtyFlag(bool on);
	// 获取undo stack
	QUndoStack* getUndoStack() const;

	// 触发dataInfomationChanged信号
	void notifyDataChangedSignal(const DAData& d, ChangeType t);

protected:
	// 设置唯一名称
	virtual void setUniqueDataName(DAData& d) const;
	// 把所有管理的变量的名字按照set返回
	QSet< QString > getDatasNameSet() const;
	// 移除数据
	void doRemoveData(DAData& d);
public Q_SLOTS:
	// 清除数据
	void clear();
Q_SIGNALS:
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
	 * @param dataOldIndex 数据原来的索引
	 */
	void dataRemoved(const DA::DAData& d, int dataOldIndex);

	/**
	 * @brief 数据信息改变
	 * @param d 数据
	 * @param oldname
	 */
	void dataChanged(const DA::DAData& d, DA::DADataManager::ChangeType t);

	/**
	 * @brief 数据被清除信号
	 */
	void datasCleared();
};
}  // namespace DA

#endif  // DADATAMANAGER_H
