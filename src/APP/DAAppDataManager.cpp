#include "DAAppDataManager.h"
#include <QList>
#include <QFileInfo>
#include <QUndoStack>
// DAPyScript
#include "DAPyScripts.h"
// DAUtils
#include "DAStringUtil.h"

namespace DA
{
class DAAppDataManagerPrivate
{
    DA_IMPL_PUBLIC(DAAppDataManager)
public:
    DAAppDataManagerPrivate(DAAppDataManager* p);
    QUndoStack _dataManagerStack;  ///< 数据管理的stack
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppDataManagerPrivate
//===================================================

DAAppDataManagerPrivate::DAAppDataManagerPrivate(DAAppDataManager* p) : q_ptr(p)
{
    _dataManagerStack.setUndoLimit(50);
}

//===================================================
// DAAppDataManager
//===================================================

DAAppDataManager::DAAppDataManager(DACoreInterface* c, QObject* p)
    : DADataManagerInterface(c, p), d_ptr(new DAAppDataManagerPrivate(this))
{
    qRegisterMetaType< DAData >("DAData");
}

DAAppDataManager::~DAAppDataManager()
{
}

/**
 * @brief 从文件导入数据
 * @param files 文件
 * @return 如果成功导入，返回导入的数量，如果返回0，说明没有导入成功
 */
int DAAppDataManager::importFromFiles(const QStringList& fileNames)
{
    qDebug() << "data manager begin import files:" << fileNames;
    QList< DAData > importDatas;
    for (const QString& f : qAsConst(fileNames)) {
        qInfo() << tr("begin import file:%1").arg(f);
        DAPyObjectWrapper res = DAPyScripts::getInstance().getIO().read(f);
        if (DAPyDataFrame::isDataFrame(res.object())) {
            qInfo() << tr("file:%1,conver to dataframe").arg(f);
            QFileInfo fi(f);
            DAPyDataFrame df = res;  //调用的是DAPyDataFrame(const DAPyObjectWrapper& df)
            DAData data      = df;
            data.setName(fi.baseName());
            data.setDescribe(fi.absoluteFilePath());
            importDatas.append(data);
        }  // else if() //其他格式
        else if (res.isNone()) {
            qWarning() << tr("can not import file:%1").arg(f);
            continue;
        }
    }
    if (importDatas.size() > 0) {
        addDatas(importDatas);
    }
    return importDatas.size();
}

/**
 * @brief 带redo/undo的添加数据
 *
 * @note 此函数会发生信号@sa dataBeginAdd 和 @sa dataEndAdded
 * @param d
 */
void DAAppDataManager::addData(DAData& d)
{
    dataManager()->addData_(d);
}

/**
 * @brief 批量添加数据
 * @param d
 */
void DAAppDataManager::addDatas(const QList< DAData >& datas)
{
    dataManager()->addDatas_(datas);
}

/**
 * @brief 带redo/undo的移除数据
 * @note 此函数会发生信号@sa dataBeginRemove 和 @sa dataEndRemoved
 * @param d
 */
void DAAppDataManager::removeData(DAData& d)
{
    dataManager()->removeData_(d);
}
/**
 * @brief 批量移除数据
 * @param datas
 */
void DAAppDataManager::removeDatas(const QList< DAData >& datas)
{
    dataManager()->removeDatas_(datas);
}

/**
 * @brief 获取undo stack
 * @return
 */
QUndoStack* DAAppDataManager::getUndoStack() const
{
    return dataManager()->getUndoStack();
}
