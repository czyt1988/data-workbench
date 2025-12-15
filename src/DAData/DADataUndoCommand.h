#ifndef DADATAUNDOCOMMAND_H
#define DADATAUNDOCOMMAND_H
#include <QUndoCommand>
#include <functional>
#include "DAData.h"
#include "DAPybind11InQt.h"
#include "DACallBackInterface.h"
namespace DA
{
class DADATA_API DADataAbstractUndoCommand : public QUndoCommand, public DACallBackInterface
{
public:
    DADataAbstractUndoCommand(QUndoCommand* par = nullptr);
    ~DADataAbstractUndoCommand();
    // 设置旧对象
    virtual void setOldData(const DAData& data) = 0;
    // 设置新对象
    virtual void setNewData(const DAData& data) = 0;
    // 设置跳过第一次redo
    void setSkipFirstRedo(bool skip = true);
    // 是否跳过第一次
    bool isSkipFirstRedo() const;
    // 跳过第一次redo
    void skipFirstRedo();

private:
    bool m_skipFirstRedo { true };  ///< 跳过第一次redo
};

/**
 * @brief 基于py::object的缓存命令
 *
 * 使用这个命令要确保内部的py::obj在前后操作不是一个对象，如果是一个对象则没有效果，因为新旧的obj都一样
 * 下面这种操作场景使用@ref DADataObjectSwapUndoCommand
 * @code{.python}
 * def dropna(dadata:da_data.DAData, col_selected_index:List[int]):
 *     df = dadata.toDataFrame()
 *     df = df.dropna(axis=0, subset=subset, thresh=thresh)
 *     dadata.setPyObject(df)
 * @endcode
 *
 * 上面这段代码DAData前后的对象已经不是一样，可以使用此命令把前后对象存起来
 *
 */
class DADATA_API DADataObjectSwapUndoCommand : public DADataAbstractUndoCommand
{
public:
    DADataObjectSwapUndoCommand(QUndoCommand* par = nullptr);
    ~DADataObjectSwapUndoCommand();
    // 设置旧对象（立即 pickle 到临时文件）
    void setOldData(const DAData& data) override;
    // 设置新对象（把新对象也pickle到临时文件）
    void setNewData(const DAData& data) override;
    void undo() override;
    void redo() override;

protected:
    DAData m_data;
    pybind11::object m_oldObject;
    pybind11::object m_newObject;
};

/**
 * @brief 文件缓存命令，此命令适合非常大的dataframe，以及前后对象是一样的场景
 *
 * 此命令的原理是setOldData时，把对象写入到临时文件中，setNewData时把对象内容写入到另外一个临时文件，命令记录新旧两个临时文件的地址，
 * 回退或重做的时候从临时文件中加载到对象来实现内容的回退
 *
 * @ref DADataUndoCommand 是针对前后对象不一样的情况，而此命令可以针对前后对象一样的场景
 * @code{.python}
 * def dropna(dadata:da_data.DAData, col_selected_index:List[int]):
 *     df = dadata.toDataFrame()
 *     df = df.dropna(axis=0, subset=subset, thresh=thresh,inplace = True)
 * @endcode
 *
 * 上面这段脚本，对DAData内部的dataframe进行操作，且原地替换内容，此时DAData内部持有的py::object是不变的，没有dadata.setPyObject设置新的对象
 *
 * 这种情况，可以使用@ref DADataFileCacheUndoCommand 直接把之前的对象内存写入到硬盘，这样就算原地改变，也能回退
 *
 * 此操作在任何情况下都能进行还原，因此如果你不清楚是否会产生新对象，你可以使用此命令
 */
class DADATA_API DADataObjectPersistUndoCommand : public DADataAbstractUndoCommand
{
public:
    DADataObjectPersistUndoCommand(QUndoCommand* par = nullptr);
    ~DADataObjectPersistUndoCommand();
    // 设置旧对象（立即 pickle 到临时文件）
    void setOldData(const DAData& data) override;
    // 设置新对象（把新对象也pickle到临时文件）
    void setNewData(const DAData& data) override;
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
    bool m_isValid { false };
};
}

#endif  // DAPYOBJECTUNDOCOMMAND_H
