#ifndef DACHART3DSERIALIZE_H
#define DACHART3DSERIALIZE_H
#include "DAFigureAPI.h"
#include <string>
#include <QIODevice>
#include <QDataStream>
#include <functional>
#include "DAChart3DPlotItemFactory.h"  // 由 03-figure-widget-3d.md 创建
#include "qwt3d_serialize.h"  // 值类型(Triple/RGBA/ParallelEpiped/Qwt3DFunctionData/Qwt3DParametricData)和 Qwt3DTheme 的 operator<</>> 由 01 实现

// 前向声明
class Qwt3DPlotItem;
class Qwt3DPlot;
class Qwt3DSurface;
class Qwt3DBar;
class Qwt3DLine;
class Qwt3DColor;
class Qwt3DStandardColor;
class Qwt3DColorMapColor;
class Qwt3DCoordinateSystem;
class Qwt3DColorLegend;

namespace DA
{
class DAChart3DWidget;

// 3D 序列化版本和 magic marks
const int gc_dachart3d_version                     = 1;
const std::uint32_t gc_dachart3d_magic_mark        = 0x3D5A6B4C;  ///< 3D item header magic
const std::uint32_t gc_dachart3d_magic_mark2       = 0x3DAA1234;  ///< 3D chart (Qwt3DPlot) magic
const std::uint32_t gc_dachart3d_magic_mark4       = 0x3DB23498;  ///< 3D item header magic (header.isValid)
const QDataStream::Version gc_dachart3d_datastream_version = QDataStream::Qt_5_12;

// figure 级别的 2D/3D chart 区分 mark
const std::uint32_t gc_dafigure_chart2d_mark = 0x2D43484F;  ///< "2DCH" — 2D chart 标识
const std::uint32_t gc_dafigure_chart3d_mark = 0x3D43484F;  ///< "3DCH" — 3D chart 标识

/**
 * @brief 针对 Qwt3DPlotItem 的二进制序列化
 *
 * 镜像 2D 的 DAChartItemSerialize 设计模式，基于 RTTI 注册的序列化/反序列化编排体系。
 * 值类型(Triple/RGBA/ParallelEpiped/Qwt3DFunctionData/Qwt3DParametricData)和 Qwt3DTheme 的
 * operator<</>> 由 01-qwt3d-enhancement.md 在 qwt3d_serialize.h 中实现(QWT3D_EXPORT)，
 * 本计划通过 #include 直接复用。
 */
class DAFIGURE_API DAChart3DItemSerialize
{
public:
    /**
     * @brief 序列化文件头，与 2D 的 Header 结构一致
     *
     * 布局：magic(4) + version(4) + rtti(4) + reserved(20) = 32 bytes
     */
    class Header
    {
    public:
        Header();
        ~Header();
        std::uint32_t magic;       ///< 魔数 --4
        int version;               ///< 版本 --8
        int rtti;                  ///< rtti --12
        unsigned char byte[ 20 ];  ///< 预留20字节，凑齐32字节
        bool isValid() const;
        friend QDataStream& operator<<(QDataStream& out, const DA::DAChart3DItemSerialize::Header& f);
        friend QDataStream& operator>>(QDataStream& in, DA::DAChart3DItemSerialize::Header& f);
    };

public:
    using FpSerializeOut = std::function< QByteArray(const Qwt3DPlotItem*) >;
    using FpSerializeIn  = std::function< Qwt3DPlotItem*(const QByteArray&) >;

public:
    DAChart3DItemSerialize();
    ~DAChart3DItemSerialize();

    static void registSerializeFun(int rtti, FpSerializeIn fpIn, FpSerializeOut fpOut);
    static bool isSupportSerialize(int rtti);
    static FpSerializeIn getSerializeInFun(int rtti) noexcept;
    static FpSerializeOut getSerializeOutFun(int rtti);

    QByteArray serializeOut(const Qwt3DPlotItem* item) const;
    Qwt3DPlotItem* serializeIn(const QByteArray& byte) const noexcept;
    int getRtti(const QByteArray& byte) const noexcept;

public:
    // 模板化的序列化实现，参考 DAChartItemSerialize::serializeIn_T / serializeOut_T
    template< typename T, int RTTI >
    static Qwt3DPlotItem* serializeIn_T(const QByteArray& byte)
    {
        return deserializeImpl< T >(byte, RTTI);
    }

    template< typename T >
    static QByteArray serializeOut_T(const Qwt3DPlotItem* item)
    {
        return serializeImpl< T >(item);
    }

private:
    template< typename T >
    static Qwt3DPlotItem* deserializeImpl(const QByteArray& byte, int expectedRtti)
    {
        QDataStream st(byte);
        st.setVersion(gc_dachart3d_datastream_version);

        Header h;
        st >> h;
        if (!h.isValid() || expectedRtti != h.rtti) {
            return nullptr;
        }

        Qwt3DPlotItem* item = DAChart3DPlotItemFactory::createItem(expectedRtti);
        if (item) {
            st >> static_cast< T* >(item);
        }
        return item;
    }

    template< typename T >
    static QByteArray serializeImpl(const Qwt3DPlotItem* item)
    {
        QByteArray byte;
        QDataStream st(&byte, QIODevice::WriteOnly);
        st.setVersion(gc_dachart3d_datastream_version);

        Header h;
        h.rtti = item->rtti();
        st << h << static_cast< const T* >(item);
        return byte;
    }

protected:
    static QHash< int, std::pair< FpSerializeIn, FpSerializeOut > >& serializeFun();
};

}  // end namespace DA

// ============================
// 值类型序列化：由 qwt3d_serialize.h（01-qwt3d-enhancement.md）提供
// Triple, RGBA, ParallelEpiped, Qwt3DFunctionData, Qwt3DParametricData
// 不在此重复声明，直接 #include "qwt3d_serialize.h" 使用
// ============================

// ============================
// 颜色 functor 序列化声明
// ============================

// Qwt3DColor 指针序列化（基类，写入类型标识 + 具体字段）
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const Qwt3DColor* c);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, Qwt3DColor*& c);

// Qwt3DStandardColor 指针序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const Qwt3DStandardColor* c);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, Qwt3DStandardColor*& c);

// Qwt3DColorMapColor 指针序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const Qwt3DColorMapColor* c);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, Qwt3DColorMapColor*& c);

// ============================
// Qwt3DPlotItem 及子类序列化声明
// ============================

// Qwt3DPlotItem 基类指针序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const Qwt3DPlotItem* item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, Qwt3DPlotItem* item);

// Qwt3DSurface 指针序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const Qwt3DSurface* item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, Qwt3DSurface* item);

// Qwt3DBar 指针序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const Qwt3DBar* item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, Qwt3DBar* item);

// Qwt3DLine 指针序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const Qwt3DLine* item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, Qwt3DLine* item);

// ============================
// Qwt3DPlot 序列化声明（item/plot 级 operator 由本计划声明和实现，DAFIGURE_API）
// 注意：01-qwt3d-enhancement.md 不声明 item/plot 级 operator，也不声明 saveState/restoreState
// ============================

// Qwt3DPlot 指针序列化（视图状态）
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const Qwt3DPlot* plot);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, Qwt3DPlot* plot);

// Qwt3DTheme 序列化由 qwt3d_serialize.h（01）提供，不在此重复声明

#endif  // DACHART3DSERIALIZE_H
