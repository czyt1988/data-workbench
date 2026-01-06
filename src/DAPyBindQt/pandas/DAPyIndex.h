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
    /**
     * @brief 适配getIndexer的方法枚举，
     */
    enum IndexerMethod
    {
        IM_None,
        IM_Ffill,
        IM_Bfill,
        IM_Nearest
    };

public:
    DAPyIndex();
    DAPyIndex(const DAPyIndex& s);
    DAPyIndex(DAPyIndex&& s);
    DAPyIndex(const pybind11::object& obj);
    DAPyIndex(pybind11::object&& obj);
    ~DAPyIndex();
    DAPyIndex& operator=(const pybind11::object& obj);
    DAPyIndex& operator=(pybind11::object&& obj);
    DAPyIndex& operator=(const DAPyIndex& obj);
    DAPyIndex& operator=(DAPyIndex&& obj);
    DAPyIndex& operator=(const DAPyObjectWrapper& obj);
    DAPyIndex& operator=(DAPyObjectWrapper&& obj);
    pybind11::object operator[](std::size_t i) const;
    pybind11::object operator[](const QSet< std::size_t >& slice) const;
    pybind11::object iat(std::size_t i) const;
    // 获取dtype
    pybind11::dtype dtype() const;
    // Index.empty
    bool empty() const;
    // Index.size
    std::size_t size() const;
    //
    QVariant value(std::size_t i) const;
    // 获取索引号
    uint64_t getIndexer(pybind11::object v, const char* method = "nearest");

protected:
    // 检测是否为pandas.index，如果不是将会设置为none
    void checkObjectValid();

public:
    static bool isIndexObj(const pybind11::object& obj);
};
}  // namespace DA
Q_DECLARE_METATYPE(DA::DAPyIndex)
#endif  // DAPYINDEX_H
