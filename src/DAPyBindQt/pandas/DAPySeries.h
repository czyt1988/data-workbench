#ifndef DAPYSERIES_H
#define DAPYSERIES_H
#include "DAPyBindQtGlobal.h"
#include "DAPyObjectWrapper.h"
#include <QDebug>
#include <QList>
#include <QVariant>
#include "DAPybind11InQt.h"
namespace DA
{
/**
 * @brief 对Pandas.Series的Qt封装
 */
class DAPYBINDQT_API DAPySeries : public DAPyObjectWrapper
{
public:
    DAPySeries() = default;
    DAPySeries(const DAPySeries& s);
    DAPySeries(DAPySeries&& s);
    DAPySeries(const pybind11::object& obj);
    DAPySeries(pybind11::object&& obj);
    ~DAPySeries();
    //操作符
    DAPySeries& operator=(const pybind11::object& obj);
    DAPySeries& operator=(const DAPySeries& s);
    QVariant operator[](std::size_t i) const;

public:
    //获取dtype
    pybind11::dtype dtype() const;
    // Series.empty
    bool empty() const;
    // Series.size
    std::size_t size() const;
    // Series.name
    QString name() const;
    // Series.iat
    QVariant iat(std::size_t i) const;
    //判断是否为series
    static bool isSeries(const pybind11::object& obj);

public:
    /**
     * @brief 把series转换为一个容器数组
     * @param begin
     */
    template< typename T, typename VectLikeIte >
    void castTo(VectLikeIte begin)
    {
        std::size_t s            = size();
        pybind11::object obj_iat = object().attr("iat");
        for (std::size_t i = 0; i < s; ++i) {
            pybind11::object obj_v = obj_iat[ pybind11::int_(i) ];
            T v                    = obj_v.cast< T >();
            *begin                 = v;
            ++begin;
        }
    }

protected:
    //检测是否为dataframe，如果不是将会设置为none
    void checkObjectValid();

public:
    //转换为文本
    QString toString(std::size_t maxele = 12) const;
};
}  // namespace DA

DAPYBINDQT_API QDebug operator<<(QDebug dbg, const DA::DAPySeries& ser);
Q_DECLARE_METATYPE(DA::DAPySeries)
#endif  // DASERIES_H
