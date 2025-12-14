#ifndef DADATAUNDOCOMMAND_H
#define DADATAUNDOCOMMAND_H
#include <QUndoCommand>
#include "DAData.h"
#include "DAPybind11InQt.h"
namespace DA
{

class DADataUndoCommand : public QUndoCommand
{
public:
    DADataUndoCommand(QUndoCommand* par = nullptr);
    ~DADataUndoCommand();
    // 设置旧对象（立即 pickle 到临时文件）
    void setOldData(const DAData& data);

    // 设置新对象（暂存为 py::object，不立即序列化）
    void setNewObject(const pybind11::object& obj);

    // 设置对象更新回调（用于通知当前状态变更）
    void setUpdateCallback(const pybind11::function& cb);

    void undo() override;
    void redo() override;

    //
    bool isValid() const;

protected:
    void dumpObj(const pybind11::object& obj, const QString& path);
    pybind11::object loadObj(const QString& path);

protected:
    DAData m_data;
    QString m_oldObjectPath;
    QString m_newObjectPath;
    pybind11::function m_updateCallback;
    bool m_isValid { false };
};
}

#endif  // DAPYOBJECTUNDOCOMMAND_H
