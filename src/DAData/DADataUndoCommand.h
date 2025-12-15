#ifndef DADATAUNDOCOMMAND_H
#define DADATAUNDOCOMMAND_H
#include <QUndoCommand>
#include <functional>
#include "DAData.h"
#include "DAPybind11InQt.h"
namespace DA
{

class DADATA_API DADataUndoCommand : public QUndoCommand
{
public:
    DADataUndoCommand(QUndoCommand* par = nullptr);
    ~DADataUndoCommand();
    // 设置旧对象（立即 pickle 到临时文件）
    void setOldData(const DAData& data);

    // 设置新对象（把新对象也pickle到临时文件）
    void setNewData(const DAData& data);

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
    bool m_skipFirstRedo { true };  ///< 跳过第一次redo
    bool m_isValid { false };
};
}

#endif  // DAPYOBJECTUNDOCOMMAND_H
