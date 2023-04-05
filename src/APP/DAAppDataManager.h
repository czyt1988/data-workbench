#ifndef DAAPPDATAMANAGER_H
#define DAAPPDATAMANAGER_H
#include <QObject>
#include "DADataManagerInterface.h"
#include <memory>
#include "DAGlobals.h"
class QUndoStack;
namespace DA
{
DA_IMPL_FORWARD_DECL(DAAppDataManager)
class DACoreInterface;
class DADataManagerTreeModel;
/**
 * @brief DA的变量管理类，da的变量统一由此类管理
 *
 * @note 方法带有redo/undo功能
 */
class DAAppDataManager : public DADataManagerInterface
{
    Q_OBJECT
    DA_IMPL(DAAppDataManager)

public:
    DAAppDataManager(DACoreInterface* c, QObject* p = nullptr);
    ~DAAppDataManager();
    //从文件导入数据,带redo/undo
    int importFromFiles(const QStringList& fileNames);
    //添加数据(支持redo/undo),返回数据的名字，添加数据时数据名字有可能会改变
    virtual void addData(DAData& d) override;
    void addDatas(const QList< DAData >& datas);
    //移除数据
    virtual void removeData(DAData& d) override;
    void removeDatas(const QList< DAData >& datas);
    //获取undo stack
    QUndoStack* getUndoStack() const;
};
}  // namespace DA

#ifndef DA_APP_DATA
/**
 * @def 获取@sa DataManager 实例
 * @note 使用此宏需要以下头文件：
 * -# DAAppCore.h
 */
#define DA_APP_DATA DA::DAAppCore::getInstance().getDatas()
#endif

#endif  // DADATAMANAGER_H
