#ifndef DAPYDTYPE_H
#define DAPYDTYPE_H
#include "DAPyBindQtGlobal.h"
#include "DAPyObjectWrapper.h"
#include <QVariant>
#include <QDebug>
#include "DAPybind11InQt.h"
namespace DA
{
/**
 * @brief 对numpy.dtype的封装
 *
 * python dtype 信息如下：
 * name=bool,kind=b,char=?,str=|b1,num=0
 * name=int8,kind=i,char=b,str=|i1,num=1
 * name=int16,kind=i,char=h,str=<i2,num=3
 * name=int32,kind=i,char=l,str=<i4,num=7
 * name=int64,kind=i,char=q,str=<i8,num=9
 * name=uint8,kind=u,char=B,str=|u1,num=2
 * name=uint16,kind=u,char=H,str=<u2,num=4
 * name=uint32,kind=u,char=L,str=<u4,num=8
 * name=uint64,kind=u,char=Q,str=<u8,num=10
 * name=float16,kind=f,char=e,str=<f2,num=23
 * name=float32,kind=f,char=f,str=<f4,num=11
 * name=float64,kind=f,char=d,str=<f8,num=12
 * name=complex64,kind=c,char=F,str=<c8,num=14
 * name=complex128,kind=c,char=D,str=<c16,num=15
 * name=str,kind=U,char=U,str=<U0,num=19
 * name=datetime64,kind=M,char=M,str=<M8,num=21
 * name=timedelta64,kind=m,char=m,str=<m8,num=22
 * name=bytes,kind=S,char=S,str=|S0,num=18
 * name=void,kind=V,char=V,str=|V0,num=20
 * name=object,kind=O,char=O,str=|O,num=17
 *
 */
class DAPYBINDQT_API DAPyDType : public DAPyObjectWrapper
{
public:
    DAPyDType() = default;
    DAPyDType(const DAPyDType& s);
    DAPyDType(DAPyDType&& s);
    DAPyDType(const pybind11::object& obj);
    DAPyDType(pybind11::object&& obj);
    DAPyDType(const pybind11::dtype& obj);
    DAPyDType(pybind11::dtype&& obj);
    DAPyDType(const QString& dtypename);
    ~DAPyDType();
    static bool isDtypeObj(const pybind11::object& obj);
    DAPyDType& operator=(const pybind11::dtype& obj);
    DAPyDType& operator=(const pybind11::object& obj);
    DAPyDType& operator=(const DAPyDType& obj);
    DAPyDType& operator=(const DAPyObjectWrapper& obj);
    bool operator==(const DAPyDType& other) const;
    bool operator!=(const DAPyDType& other) const;

public:
    //创建变量
    pybind11::object type(const QVariant& v) const;
    // dtype.name
    QString name() const;
    // dtype.kind
    char kind() const;
    // dtype.char
    char char_() const;
    // dtype.num
    int num() const;

public:
    bool isInt() const;
    bool isUInt() const;
    bool isFloat() const;
    bool isTimedelta() const;
    bool isDatetime() const;
    bool isComplex() const;
    bool isBool() const;
    bool isStr() const;
    bool isNumeral() const;

public:
    //获取所有的dtype类型名称
    static QStringList dtypeNames();

protected:
    //检测是否为numpy.dtype，如果不是将会设置为none
    void checkObjectValid();
};
}  // namespace DA
Q_DECLARE_METATYPE(DA::DAPyDType)
DAPYBINDQT_API QDebug operator<<(QDebug dbg, const DA::DAPyDType& d);
#endif  // DAPYDTYPE_H
