#ifndef DADATAMANAGERINTERFACE_H
#define DADATAMANAGERINTERFACE_H
#include "DAInterfaceAPI.h"
#include <QRegularExpression>
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

    /**
     * @brief 获取当前选中的数据，此函数要基于界面数据管理器选择的数据返回
     *
     * 当前选中的数据是指数据管理窗口正在选中的数据，如果没有选中任何数据，返回一个空列表
     * @return
     */
    virtual QList< DAData > getSelectDatas() const = 0;
    /**
     * @brief 获取当前正在操作的数据，当前正在操作的数据是指当前正在打开的表格所对应的数据
     *
     * 当前正在操作的数据是指数据操作表格正在操作的数据，如果当前没有打开任何数据，此函数返回一个空的DAData
     * @return
     */
    virtual DAData getOperateData() const = 0;

    /**
     * @brief 获取当前正在操作窗口操作的列名
     *
     * 如果用户当前正在操作一个表格，且选中了某几列，那么此函数会返回选中的列名
     *
     * 结合@ref getOperateData 和此函数，即可获取当前用户正在操作的序列
     * @sa getOperateData
     * @return 返回选中的列索引，如果没有选中任何列，返回空列表
     */
    virtual QList< int > getOperateDataSeries() const = 0;
    // 参数的索引
    int getDataIndex(const DAData& d) const;
    // 根据索引获取对应的值
    DAData getData(int index) const;
    // 根据id获取数据
    DAData getDataById(DAData::IdType id) const;
    // 查找数据
    DAData findData(const QString& name, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    // 可以使用通配符字符串查找匹配
    QList< DAData > findDatas(const QString& pattern, Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;
    QList< DAData > findDatasReg(const QRegularExpression& regex) const;
    // 获取所有数据
    QList< DAData > getAllDatas() const;
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
