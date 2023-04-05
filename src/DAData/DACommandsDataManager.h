#ifndef DACOMMANDSDATAMANAGER_H
#define DACOMMANDSDATAMANAGER_H
#include <QUndoCommand>
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
class DACommandDataManagerAdd : public QUndoCommand
{
public:
    DACommandDataManagerAdd(const DAData& d, DADataManager* mgr, QUndoCommand* par = nullptr);
    void redo() override;
    void undo() override;

private:
    DAData m_data;
    DADataManager* _dmgr;
};

/**
 * @brief 移除变量命令
 */
class DACommandDataManagerRemove : public QUndoCommand
{
public:
    DACommandDataManagerRemove(const DAData& d, DADataManager* mgr, QUndoCommand* par = nullptr);
    void redo() override;
    void undo() override;

private:
    DAData m_data;
    DADataManager* _dmgr;
};

}  // end of namespace DA
#endif  // DACOMMANDSDATAMANAGER_H
