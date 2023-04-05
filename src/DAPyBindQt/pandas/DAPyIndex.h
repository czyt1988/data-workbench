#ifndef DAPYINDEX_H
#define DAPYINDEX_H
#include "DAPyBindQtGlobal.h"
#include "DAPyObjectWrapper.h"
#include <QVariant>
#include <QSet>
#include <QDebug>
#include "DAPybind11InQt.h"
namespace DA
{
/**
 * @brief 对pandas.index的封装
 */
class DAPYBINDQT_API DAPyIndex : public DAPyObjectWrapper
{
public:
    DAPyIndex() = default;
    DAPyIndex(const DAPyIndex& s);
    DAPyIndex(DAPyIndex&& s);
    DAPyIndex(const pybind11::object& obj);
    DAPyIndex(pybind11::object&& obj);
    ~DAPyIndex();
    DAPyIndex& operator=(const pybind11::object& obj);
    DAPyIndex& operator=(const DAPyIndex& obj);
    DAPyIndex& operator=(const DAPyObjectWrapper& obj);
    QVariant operator[](std::size_t i) const;
    pybind11::object operator[](const QSet< std::size_t >& slice) const;

public:
    static bool isIndexObj(const pybind11::object& obj);

public:
    //获取dtype
    pybind11::dtype dtype() const;
    // Index.empty
    bool empty() const;
    // Index.size
    std::size_t size() const;

protected:
    //检测是否为pandas.index，如果不是将会设置为none
    void checkObjectValid();
};
}  // namespace DA
Q_DECLARE_METATYPE(DA::DAPyIndex)
#endif  // DAPYINDEX_H
