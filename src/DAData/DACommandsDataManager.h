#ifndef DACOMMANDSDATAMANAGER_H
#define DACOMMANDSDATAMANAGER_H
#include <QUndoCommand>
#include "DADataAPI.h"
#include "DAData.h"
/**
 * @file DataManager相关的命令
 *
 */
namespace DA
{
class DADataManager;
/**
 * @brief 添加变量命令
 */
class DADATA_API DACommandDataManagerAdd : public QUndoCommand
{
public:
    DACommandDataManagerAdd(const DAData& d, DADataManager* mgr, QUndoCommand* par = nullptr);
    void redo() override;
    void undo() override;

private:
    DAData mData;
    DADataManager* mDataMgr;
};

/**
 * @brief 移除变量命令
 */
class DADATA_API DACommandDataManagerRemove : public QUndoCommand
{
public:
    DACommandDataManagerRemove(const DAData& d, DADataManager* mgr, QUndoCommand* par = nullptr);
    void redo() override;
    void undo() override;

private:
    DAData mData;
    DADataManager* mDataMgr;
};

/**
 * @brief 变量重命名
 */
class DADATA_API DACommandDataManagerRenameData : public QUndoCommand
{
public:
    DACommandDataManagerRenameData(const DAData& d, const QString& newName, QUndoCommand* par = nullptr);
    void redo() override;
    void undo() override;

private:
    DAData mData;
    QString m_oldNmae;
    QString m_newName;
};

}  // end of namespace DA
#endif  // DACOMMANDSDATAMANAGER_H
