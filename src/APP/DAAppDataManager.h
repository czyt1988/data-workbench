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

/**
 * @brief DA的变量管理类，da的变量统一由此类管理
 *
 * @note 方法带有redo/undo功能
 */
class DAAppDataManager : public DADataManagerInterface
{
    Q_OBJECT
public:
    DAAppDataManager(DACoreInterface* c, QObject* p = nullptr);
    ~DAAppDataManager();
    // 从文件导入数据,带redo/undo
    bool importFromFile(const QString& f, const QVariantMap& args = QVariantMap(), QString* err = nullptr);
    int importFromFiles(const QStringList& fileNames);
    // 获取当前选中的数据
    virtual QList< DAData > getSelectDatas() const;
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
