#include "DAChart3DSerialize.h"
#include "DAChartSerialize.h"  // 复用 DABadSerializeExpection
#include <QBuffer>
#include <QDebug>
#include <cstring>
#include <QVector>
// qwt3d（qwt3d_serialize.h 由 DAChart3DSerialize.h 间接 include，提供值类型和 Theme 的 operator<</>>）
#include "qwt3d_plotitem.h"
#include "qwt3d_surface.h"
#include "qwt3d_bar.h"
#include "qwt3d_line3d.h"
#include "qwt3d_plot.h"
#include "qwt3d_color.h"
#include "qwt3d_colormap_color.h"
#include "qwt3d_types.h"
#include "qwt3d_coordsys.h"
#include "qwt3d_colorlegend.h"
// qwt
#include "qwt_point_3d.h"
#include "qwt_series_data.h"

// 值类型和 Qwt3DTheme 的 operator<</>> 由 qwt3d_serialize.h（01）提供，不在此实现

// ============================================================
// 宏定义（参考 2D 的 INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR）
// ============================================================

#ifndef INITCHART3DITEMSERIALIZE_MAKE_IN_OUT_PAIR
#define INITCHART3DITEMSERIALIZE_MAKE_IN_OUT_PAIR(RttiValue, ClassName) \
    std::make_pair(&DA::DAChart3DItemSerialize::serializeIn_T<ClassName, RttiValue>, \
                   &DA::DAChart3DItemSerialize::serializeOut_T<ClassName>)
#endif

#ifndef DECLARE_INITCHART3DITEMSERIALIZE_FUN
#define DECLARE_INITCHART3DITEMSERIALIZE_FUN(RttiValue, ClassName) \
    template Qwt3DPlotItem* DA::DAChart3DItemSerialize::serializeIn_T<ClassName, RttiValue>(const QByteArray&); \
    template QByteArray DA::DAChart3DItemSerialize::serializeOut_T<ClassName>(const Qwt3DPlotItem*);
#endif

namespace DA
{

// ============================================================
// DAChart3DItemSerialize::Header 实现
// ============================================================

/**
 * @brief 构造函数，初始化 Header 的默认值
 */
DAChart3DItemSerialize::Header::Header()
    : magic(DA::gc_dachart3d_magic_mark4)
    , version(1)
    , rtti(0)
{
    memset(byte, 0, sizeof(byte));
}

/**
 * @brief 析构函数
 */
DAChart3DItemSerialize::Header::~Header()
{
}

/**
 * @brief 判断 Header 是否有效
 * @return 若 magic 标记匹配则返回 true，否则返回 false
 */
bool DAChart3DItemSerialize::Header::isValid() const
{
    return DA::gc_dachart3d_magic_mark4 == magic;
}

/**
 * @brief 将 Header 序列化到数据流
 * @param out 输出数据流
 * @param f 要序列化的 Header
 * @return 输出数据流引用
 */
QDataStream& operator<<(QDataStream& out, const DA::DAChart3DItemSerialize::Header& f)
{
    out << f.magic << f.version << f.rtti;
    out.writeRawData(reinterpret_cast<const char*>(f.byte), sizeof(f.byte));
    return out;
}

/**
 * @brief 从数据流反序列化 Header
 * @param in 输入数据流
 * @param f 接收反序列化数据的 Header
 * @return 输入数据流引用
 */
QDataStream& operator>>(QDataStream& in, DA::DAChart3DItemSerialize::Header& f)
{
    in >> f.magic >> f.version >> f.rtti;
    in.readRawData(reinterpret_cast<char*>(f.byte), sizeof(f.byte));
    return in;
}

// ============================================================
// DAChart3DItemSerialize 类实现
// ============================================================

/**
 * @brief 构造函数
 */
DAChart3DItemSerialize::DAChart3DItemSerialize()
{
}

/**
 * @brief 析构函数
 */
DAChart3DItemSerialize::~DAChart3DItemSerialize()
{
}

/**
 * @brief 注册指定 RTTI 的序列化/反序列化函数对
 * @param rtti 运行时类型标识
 * @param fpIn 反序列化函数指针
 * @param fpOut 序列化函数指针
 * @sa isSupportSerialize, getSerializeInFun, getSerializeOutFun
 */
void DAChart3DItemSerialize::registSerializeFun(int rtti,
                                                DAChart3DItemSerialize::FpSerializeIn fpIn,
                                                DAChart3DItemSerialize::FpSerializeOut fpOut)
{
    serializeFun()[rtti] = std::make_pair(fpIn, fpOut);
}

/**
 * @brief 判断指定 RTTI 是否已注册序列化支持
 * @param rtti 运行时类型标识
 * @return 已注册返回 true，否则返回 false
 * @sa registSerializeFun
 */
bool DAChart3DItemSerialize::isSupportSerialize(int rtti)
{
    return serializeFun().contains(rtti);
}

/**
 * @brief 获取指定 RTTI 的反序列化函数
 * @param rtti 运行时类型标识
 * @return 反序列化函数指针，未注册时返回 nullptr
 * @sa registSerializeFun, getSerializeOutFun
 */
DAChart3DItemSerialize::FpSerializeIn DAChart3DItemSerialize::getSerializeInFun(int rtti) noexcept
{
    auto pair = serializeFun().value(rtti, std::make_pair<FpSerializeIn, FpSerializeOut>(nullptr, nullptr));
    return pair.first;
}

/**
 * @brief 获取指定 RTTI 的序列化函数
 * @param rtti 运行时类型标识
 * @return 序列化函数指针，未注册时返回 nullptr
 * @sa registSerializeFun, getSerializeInFun
 */
DAChart3DItemSerialize::FpSerializeOut DAChart3DItemSerialize::getSerializeOutFun(int rtti)
{
    auto pair = serializeFun().value(rtti, std::make_pair<FpSerializeIn, FpSerializeOut>(nullptr, nullptr));
    return pair.second;
}

/**
 * @brief 序列化 3D 绘图项到字节数组
 * @param item 要序列化的 Qwt3DPlotItem 指针
 * @return 序列化后的 QByteArray，若 RTTI 未注册则返回空数组
 * @sa serializeIn, getSerializeOutFun
 */
QByteArray DAChart3DItemSerialize::serializeOut(const Qwt3DPlotItem* item) const
{
    int rtti = item->rtti();
    FpSerializeOut fp = getSerializeOutFun(rtti);
    if (!fp) {
        qDebug() << QString("While serializing the 3D plot item, an unregistered RTTI value (%1) was encountered.")
                        .arg(rtti);  // cn:序列化3D plotitem时，遇到未注册的RTTI值(%1)
        return QByteArray();
    }
    return fp(item);
}

/**
 * @brief 从字节数组反序列化 3D 绘图项
 * @param byte 包含序列化数据的字节数组
 * @return 反序列化后的 Qwt3DPlotItem 指针，失败时返回 nullptr
 * @sa serializeOut, getSerializeInFun, getRtti
 */
Qwt3DPlotItem* DAChart3DItemSerialize::serializeIn(const QByteArray& byte) const noexcept
{
    int rtti = getRtti(byte);
    if (rtti < 0) {
        return nullptr;
    }
    FpSerializeIn fp = getSerializeInFun(rtti);
    if (!fp) {
        qDebug() << QString("While deserializing the 3D plot item, an unregistered RTTI value (%1) was encountered.")
                        .arg(rtti);  // cn:反序列化3D plotitem时，遇到未注册的RTTI值(%1)
        return nullptr;
    }
    Qwt3DPlotItem* item = nullptr;
    try {
        item = fp(byte);
    } catch (const std::exception& e) {
        qDebug() << e.what();
    }
    return item;
}

/**
 * @brief 从字节数组中提取 RTTI 类型标识
 * @param byte 包含序列化数据的字节数组
 * @return RTTI 值，若数据无效则返回 -1
 * @sa serializeIn, Header
 */
int DAChart3DItemSerialize::getRtti(const QByteArray& byte) const noexcept
{
    int rtti = -1;
    try {
        QBuffer buffer;
        buffer.setData(byte);
        buffer.open(QIODevice::ReadOnly);
        QDataStream st(&buffer);
        DAChart3DItemSerialize::Header h;
        st.setVersion(gc_dachart3d_datastream_version);
        st >> h;
        if (h.isValid()) {
            rtti = h.rtti;
        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
        return -1;
    }
    return rtti;
}

// === 显式模板实例化 + 注册 ===

DECLARE_INITCHART3DITEMSERIALIZE_FUN(Rtti_Plot3DSurface, Qwt3DSurface)
DECLARE_INITCHART3DITEMSERIALIZE_FUN(Rtti_Plot3DBar, Qwt3DBar)
DECLARE_INITCHART3DITEMSERIALIZE_FUN(Rtti_Plot3DLine, Qwt3DLine)

/**
 * @brief 初始化 3D 绘图项的序列化函数映射表
 * @return 包含所有已注册 RTTI 及其序列化/反序列化函数对的 QHash
 * @sa serializeFun
 */
QHash<int, std::pair<DAChart3DItemSerialize::FpSerializeIn, DAChart3DItemSerialize::FpSerializeOut>>
initChart3DItemSerialize()
{
    QHash<int, std::pair<DAChart3DItemSerialize::FpSerializeIn, DAChart3DItemSerialize::FpSerializeOut>> res;
    res[Rtti_Plot3DSurface] = INITCHART3DITEMSERIALIZE_MAKE_IN_OUT_PAIR(Rtti_Plot3DSurface, Qwt3DSurface);
    res[Rtti_Plot3DBar] = INITCHART3DITEMSERIALIZE_MAKE_IN_OUT_PAIR(Rtti_Plot3DBar, Qwt3DBar);
    res[Rtti_Plot3DLine] = INITCHART3DITEMSERIALIZE_MAKE_IN_OUT_PAIR(Rtti_Plot3DLine, Qwt3DLine);
    return res;
}

/**
 * @brief 获取序列化函数映射表（静态单例）
 * @return 序列化函数映射表的引用
 * @sa initChart3DItemSerialize, registSerializeFun
 */
QHash<int, std::pair<DAChart3DItemSerialize::FpSerializeIn, DAChart3DItemSerialize::FpSerializeOut>>&
DAChart3DItemSerialize::serializeFun()
{
    static QHash<int, std::pair<FpSerializeIn, FpSerializeOut>> s_serializeMap = initChart3DItemSerialize();
    return s_serializeMap;
}

}  // end namespace DA

// ============================================================
// 颜色 functor 序列化（步骤 5）
// ============================================================

// ============================
// Qwt3DColor 基类指针序列化
// ============================
/**
 * @brief 序列化 Qwt3DColor 基类指针到数据流
 * @param out 输出数据流
 * @param c 颜色 functor 指针
 * @return 输出数据流引用
 * @sa operator>>(QDataStream&, Qwt3DColor*&)
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DColor* c)
{
    out << DA::gc_dachart3d_version << DA::gc_dachart3d_magic_mark;
    if (c == nullptr) {
        out << static_cast<int>(0);  // 无颜色 functor
        return out;
    }
    // 尝试动态类型识别
    const Qwt3DStandardColor* sc = dynamic_cast<const Qwt3DStandardColor*>(c);
    const Qwt3DColorMapColor*  mc = dynamic_cast<const Qwt3DColorMapColor*>(c);
    if (sc) {
        out << static_cast<int>(1) << static_cast<const Qwt3DStandardColor*>(c);
    } else if (mc) {
        out << static_cast<int>(2) << static_cast<const Qwt3DColorMapColor*>(c);
    } else {
        out << static_cast<int>(0);  // 未知类型，当作无 functor
    }
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DColor 基类指针
 * @param in 输入数据流
 * @param c 接收反序列化颜色 functor 的指针引用
 * @return 输入数据流引用
 * @sa operator<<(QDataStream&, const Qwt3DColor*)
 */
QDataStream& operator>>(QDataStream& in, Qwt3DColor*& c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart3d_magic_mark != magic) {
        throw DA::DABadSerializeExpection("Qwt3DColor: invalid magic mark");
    }
    int typeId = 0;
    in >> typeId;
    if (c) {
        c->destroy();  // 释放旧对象
        c = nullptr;
    }
    switch (typeId) {
    case 0:
        c = nullptr;
        break;
    case 1: {
        Qwt3DStandardColor* sc = nullptr;
        in >> sc;
        c = sc;
        break;
    }
    case 2: {
        Qwt3DColorMapColor* mc = nullptr;
        in >> mc;
        c = mc;
        break;
    }
    default:
        c = nullptr;
        break;
    }
    return in;
}

// ============================
// Qwt3DStandardColor 指针序列化
// ============================

/**
 * @brief 序列化 Qwt3DStandardColor 到数据流
 * @details 可序列化字段：presetName, colorCount, alpha。
 * 已知限制：自定义（非预设）颜色配置不会保留，反序列化时从预设名重建。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DStandardColor* c)
{
    out << DA::gc_dachart3d_version << DA::gc_dachart3d_magic_mark2;
    QString presetName = c->presetName();
    unsigned size = c->colorCount();
    double alpha = c->alpha();
    out << presetName << static_cast<quint32>(size) << alpha;
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DStandardColor 指针
 * @param in 输入数据流
 * @param c 接收反序列化颜色 functor 的指针引用
 * @return 输入数据流引用
 * @sa operator<<(QDataStream&, const Qwt3DStandardColor*)
 */
QDataStream& operator>>(QDataStream& in, Qwt3DStandardColor*& c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart3d_magic_mark2 != magic) {
        throw DA::DABadSerializeExpection("Qwt3DStandardColor: invalid magic mark");
    }
    QString presetName;
    quint32 size = 0;
    double alpha = 1.0;
    in >> presetName >> size >> alpha;
    if (c) {
        c->destroy();
    }
    c = new Qwt3DStandardColor(static_cast<unsigned>(size));
    if (!presetName.isEmpty()) {
        c->setPreset(presetName, static_cast<unsigned>(size));
    }
    c->setAlpha(alpha);
    return in;
}

// ============================
// Qwt3DColorMapColor 指针序列化
// ============================

/**
 * @brief 序列化 Qwt3DColorMapColor 到数据流
 * @details 可序列化字段：presetName, colorCount, alpha, useManualInterval, manualMin, manualMax。
 * 已知限制：自定义（非预设）颜色配置不会保留，反序列化时从预设名重建。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DColorMapColor* c)
{
    out << DA::gc_dachart3d_version << DA::gc_dachart3d_magic_mark2;
    QString presetName = c->presetName();
    unsigned size = c->colorCount();
    double alpha = c->alpha();
    bool useManual = c->useManualInterval();
    double manualMin = c->manualMin();
    double manualMax = c->manualMax();
    out << presetName << static_cast<quint32>(size) << alpha
        << useManual << manualMin << manualMax;
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DColorMapColor 指针
 * @param in 输入数据流
 * @param c 接收反序列化颜色 functor 的指针引用
 * @return 输入数据流引用
 * @sa operator<<(QDataStream&, const Qwt3DColorMapColor*)
 */
QDataStream& operator>>(QDataStream& in, Qwt3DColorMapColor*& c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart3d_magic_mark2 != magic) {
        throw DA::DABadSerializeExpection("Qwt3DColorMapColor: invalid magic mark");
    }
    QString presetName;
    quint32 size = 0;
    double alpha = 1.0;
    bool useManual = false;
    double manualMin = 0.0;
    double manualMax = 1.0;
    in >> presetName >> size >> alpha >> useManual >> manualMin >> manualMax;
    if (c) {
        c->destroy();
    }
    c = new Qwt3DColorMapColor(presetName, static_cast<unsigned>(size));
    c->setAlpha(alpha);
    if (useManual) {
        c->setInterval(manualMin, manualMax);
    }
    return in;
}

// ============================================================
// Qwt3DPlotItem 基类序列化（步骤 6）
// ============================================================

/**
 * @brief Qwt3DPlotItem 基类指针序列化
 * @details 序列化基类属性：title, rtti, z, visible。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DPlotItem* item)
{
    out << DA::gc_dachart3d_version << DA::gc_dachart3d_magic_mark;
    out << item->rtti();  // 写入 rtti 用于类型识别
    out << item->title() << item->z() << item->isVisible();
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DPlotItem 基类属性
 * @param in 输入数据流
 * @param item 接收反序列化数据的绘图项指针
 * @return 输入数据流引用
 * @sa operator<<(QDataStream&, const Qwt3DPlotItem*)
 */
QDataStream& operator>>(QDataStream& in, Qwt3DPlotItem* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart3d_magic_mark != magic) {
        throw DA::DABadSerializeExpection("Qwt3DPlotItem: invalid magic mark");
    }
    int rtti;
    QString title;
    double z;
    bool visible;
    in >> rtti >> title >> z >> visible;
    item->setTitle(title);
    item->setZ(z);
    item->setVisible(visible);
    return in;
}

// ============================================================
// Qwt3DSurface 序列化（步骤 7）
// ============================================================

/**
 * @brief Qwt3DSurface 指针序列化
 * @details 序列化完整属性：plotStyle, color functor, mesh 属性, isolines,
 * floorStyle, normals, shading, resolution, polygonOffset, 以及 grid/cell 数据。
 * @note 直接访问 Qwt3DGridData::vertices 的 double* 内部表示（Vertex = double*，
 * 指向 3 个连续 double）。这些 Vertex 由 Qwt3DGridData 拥有，生命周期与 Surface item
 * 绑定，在序列化过程中不会释放，因此直接访问是安全的。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DSurface* item)
{
    out << DA::gc_dachart3d_version << DA::gc_dachart3d_magic_mark;
    // 写入基类
    out << static_cast<const Qwt3DPlotItem*>(item);
    // 写入 plot style
    out << static_cast<int>(item->plotStyle());
    // 写入颜色 functor
    const Qwt3DColor* colorFunc = item->dataColor();
    out << colorFunc;  // 使用 Qwt3DColor* 的 operator<<
    // 写入 mesh 属性
    RGBA meshColor = item->meshColor();
    out << meshColor << item->meshLineWidth();
    // 写入 isolines
    out << item->isolines() << item->smoothMesh();
    // 写入 floor style
    out << static_cast<int>(item->floorStyle());
    // 写入 normals
    out << item->normals() << item->normalLength() << item->normalQuality();
    // 写入 shading
    out << static_cast<int>(item->shading());
    // 写入 resolution 和 polygonOffset
    out << item->resolution() << item->polygonOffset();

    // 写入数据
    if (item->isGridData()) {
        out << static_cast<int>(1);  // grid data 标识
        Qwt3DGridData* gd = item->gridData();
        if (gd) {
            // 使用 vertices 公有成员获取维度（Qwt3DGridData::columns()/rows() 未从 DLL 导出）
            int cols = static_cast<int>(gd->vertices.size());
            int rows = (cols > 0) ? static_cast<int>(gd->vertices[0].size()) : 0;
            out << cols << rows;
            // 写入 vertices 矩阵
            // DataMatrix = std::vector<DataRow>, DataRow = std::vector<double*>
            // 每个 Vertex 是 double[3] (x, y, z)
            // Vertex 由 Qwt3DGridData 拥有，生命周期与 Surface item 绑定，序列化过程中安全
            for (int i = 0; i < cols; ++i) {
                for (int j = 0; j < rows; ++j) {
                    out << gd->vertices[static_cast<unsigned>(i)][static_cast<unsigned>(j)][0]
                        << gd->vertices[static_cast<unsigned>(i)][static_cast<unsigned>(j)][1]
                        << gd->vertices[static_cast<unsigned>(i)][static_cast<unsigned>(j)][2];
                }
            }
            // periodic 标志：uperiodic()/vperiodic() 未从 DLL 导出，写默认值 false
            // 已知限制：周期性标志不保留，反序列化时使用默认值
            out << false << false;
        } else {
            out << 0 << 0;  // 空数据
        }
    } else {
        out << static_cast<int>(2);  // cell data 标识
        Qwt3DCellData* cd = item->cellData();
        if (cd) {
            // 写入 nodes (TripleField = std::vector<Triple>)
            out << static_cast<quint32>(cd->nodes.size());
            for (const Triple& t : cd->nodes) {
                out << t;
            }
            // 写入 cells (CellField = std::vector<Cell>, Cell = std::vector<unsigned>)
            out << static_cast<quint32>(cd->cells.size());
            for (const Cell& cell : cd->cells) {
                out << static_cast<quint32>(cell.size());
                for (unsigned idx : cell) {
                    out << idx;
                }
            }
        } else {
            out << static_cast<quint32>(0) << static_cast<quint32>(0);  // 空数据
        }
    }
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DSurface 指针
 * @param in 输入数据流
 * @param item 接收反序列化数据的 Surface 指针
 * @return 输入数据流引用
 * @sa operator<<(QDataStream&, const Qwt3DSurface*)
 */
QDataStream& operator>>(QDataStream& in, Qwt3DSurface* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart3d_magic_mark != magic) {
        throw DA::DABadSerializeExpection("Qwt3DSurface: invalid magic mark");
    }
    // 读取基类
    in >> static_cast<Qwt3DPlotItem*>(item);
    // 读取 plot style
    int plotStyle;
    in >> plotStyle;
    item->setPlotStyle(static_cast<PLOTSTYLE>(plotStyle));
    // 读取颜色 functor
    Qwt3DColor* colorFunc = nullptr;
    in >> colorFunc;
    if (colorFunc) {
        item->setDataColor(colorFunc);
    }
    // 读取 mesh 属性
    RGBA meshColor;
    double meshLineWidth;
    in >> meshColor >> meshLineWidth;
    item->setMeshColor(meshColor);
    item->setMeshLineWidth(meshLineWidth);
    // 读取 isolines
    int isolines;
    bool smoothMesh;
    in >> isolines >> smoothMesh;
    item->setIsolines(isolines);
    item->setSmoothMesh(smoothMesh);
    // 读取 floor style
    int floorStyle;
    in >> floorStyle;
    item->setFloorStyle(static_cast<FLOORSTYLE>(floorStyle));
    // 读取 normals
    bool showNormals;
    double normalLength;
    int normalQuality;
    in >> showNormals >> normalLength >> normalQuality;
    item->showNormals(showNormals);
    item->setNormalLength(normalLength);
    item->setNormalQuality(normalQuality);
    // 读取 shading
    int shading;
    in >> shading;
    item->setShading(static_cast<SHADINGSTYLE>(shading));
    // 读取 resolution 和 polygonOffset
    int resolution;
    double polygonOffset;
    in >> resolution >> polygonOffset;
    item->setResolution(resolution);
    item->setPolygonOffset(polygonOffset);

    // 读取数据
    int dataType;
    in >> dataType;
    if (dataType == 1) {
        // grid data
        int cols, rows;
        in >> cols >> rows;
        if (cols > 0 && rows > 0) {
            // 读取 vertices 矩阵，构建 Triple 数组
            std::vector<std::vector<Triple>> tripleMatrix(static_cast<size_t>(cols),
                                                          std::vector<Triple>(static_cast<size_t>(rows)));
            for (int i = 0; i < cols; ++i) {
                for (int j = 0; j < rows; ++j) {
                    in >> tripleMatrix[static_cast<size_t>(i)][static_cast<size_t>(j)];
                }
            }
            bool uperiodic, vperiodic;
            in >> uperiodic >> vperiodic;
            // 转换为 Triple** 数组
            std::vector<Triple*> ptrCols(static_cast<size_t>(cols));
            for (int i = 0; i < cols; ++i) {
                ptrCols[static_cast<size_t>(i)] = tripleMatrix[static_cast<size_t>(i)].data();
            }
            item->loadFromData(ptrCols.data(), static_cast<unsigned>(cols), static_cast<unsigned>(rows),
                               uperiodic, vperiodic);
        }
    } else if (dataType == 2) {
        // cell data
        quint32 nodeCount = 0;
        in >> nodeCount;
        TripleField nodes(static_cast<size_t>(nodeCount));
        for (quint32 i = 0; i < nodeCount; ++i) {
            in >> nodes[i];
        }
        quint32 cellCount = 0;
        in >> cellCount;
        CellField cells(static_cast<size_t>(cellCount));
        for (quint32 i = 0; i < cellCount; ++i) {
            quint32 cellSize = 0;
            in >> cellSize;
            cells[i].resize(cellSize);
            for (quint32 j = 0; j < cellSize; ++j) {
                in >> cells[i][j];
            }
        }
        if (!nodes.empty()) {
            item->loadFromData(nodes, cells);
        }
    }
    return in;
}

// ============================================================
// Qwt3DBar 序列化（步骤 8）
// ============================================================

/**
 * @brief Qwt3DBar 指针序列化
 * @details 序列化完整属性：barStyle, bar 几何参数, color functor, mesh 属性,
 * 以及 1D/2D 数据。使用 gridZValues() 获取原始 z 矩阵，避免 m_bars 排序假设。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DBar* item)
{
    out << DA::gc_dachart3d_version << DA::gc_dachart3d_magic_mark;
    // 写入基类
    out << static_cast<const Qwt3DPlotItem*>(item);
    // 写入 bar style
    out << static_cast<int>(item->barStyle());
    // 写入 bar 几何参数
    out << item->barWidth() << item->barDepth() << item->baseline();
    // 写入颜色 functor
    const Qwt3DColor* colorFunc = item->dataColor();
    out << colorFunc;
    // 写入 mesh 属性
    RGBA meshColor = item->meshColor();
    out << meshColor << item->meshLineWidth();
    // 写入数据
    if (item->isGridData()) {
        out << static_cast<int>(2);  // 2D grid data 标识
        int cols = item->gridColumns();
        int rows = item->gridRows();
        double minX = item->gridMinX();
        double maxX = item->gridMaxX();
        double minY = item->gridMinY();
        double maxY = item->gridMaxY();
        out << cols << rows << minX << maxX << minY << maxY;
        // 直接使用 gridZValues() 获取原始 z 矩阵
        std::vector<std::vector<double>> zValues = item->gridZValues();
        for (int i = 0; i < cols; ++i) {
            for (int j = 0; j < rows; ++j) {
                if (static_cast<size_t>(i) < zValues.size()
                    && static_cast<size_t>(j) < zValues[static_cast<size_t>(i)].size()) {
                    out << zValues[static_cast<size_t>(i)][static_cast<size_t>(j)];
                } else {
                    out << 0.0;
                }
            }
        }
    } else {
        out << static_cast<int>(1);  // 1D series data 标识
        const QVector<QwtPoint3D> samples = item->samples();
        out << static_cast<quint64>(samples.size());
        for (const auto& s : samples) {
            out << s.x() << s.y() << s.z();
        }
    }
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DBar 指针
 * @param in 输入数据流
 * @param item 接收反序列化数据的 Bar 指针
 * @return 输入数据流引用
 * @sa operator<<(QDataStream&, const Qwt3DBar*)
 */
QDataStream& operator>>(QDataStream& in, Qwt3DBar* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart3d_magic_mark != magic) {
        throw DA::DABadSerializeExpection("Qwt3DBar: invalid magic mark");
    }
    // 读取基类
    in >> static_cast<Qwt3DPlotItem*>(item);
    // 读取 bar style
    int barStyle;
    in >> barStyle;
    item->setBarStyle(static_cast<Qwt3DBar::BarStyle>(barStyle));
    // 读取 bar 几何参数
    double barWidth, barDepth, baseline;
    in >> barWidth >> barDepth >> baseline;
    item->setBarWidth(barWidth);
    item->setBarDepth(barDepth);
    item->setBaseline(baseline);
    // 读取颜色 functor
    Qwt3DColor* colorFunc = nullptr;
    in >> colorFunc;
    if (colorFunc) {
        item->setDataColor(colorFunc);
    }
    // 读取 mesh 属性
    RGBA meshColor;
    double meshLineWidth;
    in >> meshColor >> meshLineWidth;
    item->setMeshColor(meshColor);
    item->setMeshLineWidth(meshLineWidth);
    // 读取数据
    int dataType;
    in >> dataType;
    if (dataType == 1) {
        // 1D series
        quint64 sampleCount = 0;
        in >> sampleCount;
        QVector<QwtPoint3D> samples;
        samples.reserve(static_cast<int>(sampleCount));
        for (quint64 i = 0; i < sampleCount; ++i) {
            double x, y, z;
            in >> x >> y >> z;
            samples.append(QwtPoint3D(x, y, z));
        }
        item->setSamples(samples);
    } else if (dataType == 2) {
        // 2D grid
        int cols, rows;
        double minX, maxX, minY, maxY;
        in >> cols >> rows >> minX >> maxX >> minY >> maxY;
        // 读取 z 矩阵
        Qwt3DFunctionData data;
        data.columns = static_cast<unsigned>(cols);
        data.rows = static_cast<unsigned>(rows);
        data.minx = minX;
        data.maxx = maxX;
        data.miny = minY;
        data.maxy = maxY;
        data.z.resize(static_cast<size_t>(cols));
        for (int i = 0; i < cols; ++i) {
            data.z[static_cast<size_t>(i)].resize(static_cast<size_t>(rows));
            for (int j = 0; j < rows; ++j) {
                in >> data.z[static_cast<size_t>(i)][static_cast<size_t>(j)];
            }
        }
        item->setSamples(data);
    }
    return in;
}

// ============================================================
// Qwt3DLine 序列化（步骤 9）
// ============================================================

/**
 * @brief Qwt3DLine 指针序列化
 * @details 序列化完整属性：lineStyle, line 参数, point 参数, color, 以及 samples 数据。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DLine* item)
{
    out << DA::gc_dachart3d_version << DA::gc_dachart3d_magic_mark;
    // 写入基类
    out << static_cast<const Qwt3DPlotItem*>(item);
    // 写入 line style
    out << static_cast<int>(item->lineStyle());
    // 写入 line 参数
    out << item->lineWidth() << item->tubeRadius() << item->tubeSegments();
    // 写入 point 参数
    out << item->pointSize() << item->pointVisible() << static_cast<int>(item->pointShape());
    // 写入颜色
    const Qwt3DColor* colorFunc = item->dataColor();
    bool hasDataColor = (colorFunc != nullptr);
    out << hasDataColor;
    if (hasDataColor) {
        out << colorFunc;
    } else {
        // 写入 solid color
        RGBA solidColor = item->color();
        out << solidColor;
    }
    // 写入数据 (samples)
    const QwtSeriesData<QwtPoint3D>* seriesData = item->data();
    size_t sampleCount = item->dataSize();
    out << static_cast<quint64>(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        QwtPoint3D sample = seriesData->sample(static_cast<size_t>(i));
        out << sample.x() << sample.y() << sample.z();
    }
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DLine 指针
 * @param in 输入数据流
 * @param item 接收反序列化数据的 Line 指针
 * @return 输入数据流引用
 * @sa operator<<(QDataStream&, const Qwt3DLine*)
 */
QDataStream& operator>>(QDataStream& in, Qwt3DLine* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart3d_magic_mark != magic) {
        throw DA::DABadSerializeExpection("Qwt3DLine: invalid magic mark");
    }
    // 读取基类
    in >> static_cast<Qwt3DPlotItem*>(item);
    // 读取 line style
    int lineStyle;
    in >> lineStyle;
    item->setLineStyle(static_cast<Qwt3DLine::LineStyle>(lineStyle));
    // 读取 line 参数
    double lineWidth, tubeRadius;
    int tubeSegments;
    in >> lineWidth >> tubeRadius >> tubeSegments;
    item->setLineWidth(lineWidth);
    item->setTubeRadius(tubeRadius);
    item->setTubeSegments(tubeSegments);
    // 读取 point 参数
    double pointSize;
    bool pointVisible;
    int pointShape;
    in >> pointSize >> pointVisible >> pointShape;
    item->setPointSize(pointSize);
    item->setPointVisible(pointVisible);
    item->setPointShape(static_cast<Qwt3DLine::PointShape>(pointShape));
    // 读取颜色
    bool hasDataColor;
    in >> hasDataColor;
    if (hasDataColor) {
        Qwt3DColor* colorFunc = nullptr;
        in >> colorFunc;
        if (colorFunc) {
            item->setDataColor(colorFunc);
        }
    } else {
        RGBA solidColor;
        in >> solidColor;
        item->setColor(solidColor);
    }
    // 读取数据
    quint64 sampleCount = 0;
    in >> sampleCount;
    QVector<QwtPoint3D> samples;
    samples.reserve(static_cast<int>(sampleCount));
    for (quint64 i = 0; i < sampleCount; ++i) {
        double x, y, z;
        in >> x >> y >> z;
        samples.append(QwtPoint3D(x, y, z));
    }
    if (!samples.isEmpty()) {
        item->setSamples(samples);
    }
    return in;
}

// ============================================================
// Qwt3DPlot 序列化（步骤 11）
// ============================================================

/**
 * @brief Qwt3DPlot 指针序列化（视图状态）
 * @details 序列化视图状态 + 标题 + 坐标系/轴设置 + item 列表。
 * item 列表通过 DAChart3DItemSerialize 逐个序列化，每个 item 的数据作为一个 QByteArray 写入。
 * operator<</>> 直接调用 getter/setter，不委托 saveState()/restoreState()。
 *
 * 已知限制：
 * - 网格线开关（setGridLines 中的 majors/minors bool 标志）无公有 getter，不在此序列化。
 * - 颜色属性由 Qwt3DTheme 覆盖，不在此重复序列化。
 * - Qwt3DColorLegend 的完整属性序列化依赖 01 添加 getter 后扩展。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DPlot* plot)
{
    out << DA::gc_dachart3d_version << DA::gc_dachart3d_magic_mark2;
    // 视图状态：旋转
    out << plot->xRotation() << plot->yRotation() << plot->zRotation();
    // 视图状态：平移
    out << plot->xShift() << plot->yShift() << plot->zShift();
    // 视图状态：视口平移
    out << plot->xViewportShift() << plot->yViewportShift();
    // 视图状态：缩放
    out << plot->xScale() << plot->yScale() << plot->zScale();
    // 视图状态：zoom
    out << plot->zoom();
    // 视图状态：投影模式
    out << plot->ortho();
    // 视图状态：纵横比模式
    out << static_cast<int>(plot->aspectRatioMode());
    // 背景色
    out << plot->backgroundRGBAColor();
    // 标题
    out << plot->title();
    // 主题
    out << plot->theme();
    // 光照
    out << plot->lightingEnabled();
    out << plot->xLightRotation() << plot->yLightRotation() << plot->zLightRotation();
    out << plot->xLightShift() << plot->yLightShift() << plot->zLightShift();
    // 颜色图例显隐状态
    bool hasLegend = plot->isColorLegendShown();
    out << hasLegend;
    // TODO(01): 01 添加 ColorLegend getter 后，在此处序列化完整图例属性
    // 坐标系设置（非颜色属性，颜色由 theme 覆盖）
    // coordinates() 是非 const 方法（qwt3d_plot.h:50），但实际不修改对象状态，
    // const_cast 安全（m_coordinates 是值成员，coordinates() 返回 &d->m_coordinates）
    Qwt3DCoordinateSystem* coordSys = const_cast<Qwt3DPlot*>(plot)->coordinates();
    out << static_cast<int>(coordSys->style());  // COORDSTYLE
    double ticMajor, ticMinor;
    coordSys->ticLength(ticMajor, ticMinor);
    out << ticMajor << ticMinor;
    out << coordSys->autoScale();
    out << coordSys->lineSmooth() << coordSys->autoDecoration();
    out << static_cast<int>(coordSys->tickPosition());
    // 序列化 12 根轴的关键设置（颜色属性由 theme 覆盖，不在此重复序列化）
    // coordSys->axes 是 std::vector<Qwt3DAxis>，是 const 容器（const 访问），可直接范围迭代
    for (const Qwt3DAxis& axis : coordSys->axes) {
        out << axis.labelString()
            << axis.majors() << axis.minors()
            << axis.labelFont() << axis.numberFont()
            << axis.scaling() << axis.numbers()
            << axis.autoScale();
        double limStart, limStop;
        axis.limits(limStart, limStop);
        out << limStart << limStop;
    }
    // item 列表 (通过 DAChart3DItemSerialize 逐个序列化)
    const QList<Qwt3DPlotItem*>& items = plot->itemList();
    out << static_cast<quint32>(items.size());
    DA::DAChart3DItemSerialize serializer;
    for (Qwt3DPlotItem* item : items) {
        QByteArray itemData = serializer.serializeOut(item);
        out << itemData;
    }
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DPlot 视图状态
 * @param in 输入数据流
 * @param plot 接收反序列化数据的 3D 绘图指针
 * @return 输入数据流引用
 * @sa operator<<(QDataStream&, const Qwt3DPlot*)
 */
QDataStream& operator>>(QDataStream& in, Qwt3DPlot* plot)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart3d_magic_mark2 != magic) {
        throw DA::DABadSerializeExpection("Qwt3DPlot: invalid magic mark");
    }
    // 读取视图状态：旋转
    double xRot, yRot, zRot;
    in >> xRot >> yRot >> zRot;
    plot->setRotation(xRot, yRot, zRot);
    // 读取视图状态：平移
    double xShift, yShift, zShift;
    in >> xShift >> yShift >> zShift;
    plot->setShift(xShift, yShift, zShift);
    // 读取视图状态：视口平移
    double xVpShift, yVpShift;
    in >> xVpShift >> yVpShift;
    plot->setViewportShift(xVpShift, yVpShift);
    // 读取视图状态：缩放
    double xScale, yScale, zScale;
    in >> xScale >> yScale >> zScale;
    plot->setScale(xScale, yScale, zScale);
    // 读取视图状态：zoom
    double zoom;
    in >> zoom;
    plot->setZoom(zoom);
    // 读取视图状态：投影模式
    bool ortho;
    in >> ortho;
    plot->setOrtho(ortho);
    // 读取视图状态：纵横比模式
    int aspectRatioMode;
    in >> aspectRatioMode;
    plot->setAspectRatioMode(static_cast<ASPECTRATIOMODE>(aspectRatioMode));
    // 读取背景色
    RGBA bgColor;
    in >> bgColor;
    plot->setBackgroundColor(bgColor);
    // 读取标题
    QString titleStr;
    in >> titleStr;
    plot->setTitle(titleStr);
    // 读取主题
    Qwt3DTheme theme;
    in >> theme;
    plot->setTheme(theme);
    // 读取光照
    bool lightingEnabled;
    in >> lightingEnabled;
    plot->enableLighting(lightingEnabled);
    double xLightRot, yLightRot, zLightRot;
    double xLightShift, yLightShift, zLightShift;
    in >> xLightRot >> yLightRot >> zLightRot;
    in >> xLightShift >> yLightShift >> zLightShift;
    plot->setLightRotation(xLightRot, yLightRot, zLightRot);
    plot->setLightShift(xLightShift, yLightShift, zLightShift);
    // 读取颜色图例显隐状态
    bool hasLegend;
    in >> hasLegend;
    plot->showColorLegend(hasLegend);
    // 读取坐标系设置
    int coordStyle;
    in >> coordStyle;
    Qwt3DCoordinateSystem* coordSys = plot->coordinates();
    coordSys->setStyle(static_cast<COORDSTYLE>(coordStyle));
    double ticMajor, ticMinor;
    in >> ticMajor >> ticMinor;
    coordSys->setTicLength(ticMajor, ticMinor);
    bool autoScale;
    in >> autoScale;
    coordSys->setAutoScale(autoScale);
    bool lineSmooth, autoDecoration;
    int tickPosition;
    in >> lineSmooth >> autoDecoration >> tickPosition;
    coordSys->setLineSmooth(lineSmooth);
    coordSys->setAutoDecoration(autoDecoration);
    coordSys->setTickPosition(static_cast<TICKPOSITION>(tickPosition));
    // 读取 12 根轴的关键设置
    for (size_t i = 0; i < coordSys->axes.size(); ++i) {
        Qwt3DAxis& axis = coordSys->axes[i];
        QString labelStr;
        int majors, minors;
        QFont labelFont, numberFont;
        bool scaling, numbers, autoScaleAxis;
        double limStart, limStop;
        in >> labelStr >> majors >> minors >> labelFont >> numberFont
           >> scaling >> numbers >> autoScaleAxis >> limStart >> limStop;
        axis.setLabelString(labelStr);
        axis.setMajors(majors);
        axis.setMinors(minors);
        axis.setLabelFont(labelFont);
        axis.setNumberFont(numberFont);
        axis.setScaling(scaling);
        axis.setNumbers(numbers);
        axis.setAutoScale(autoScaleAxis);
        axis.setLimits(limStart, limStop);
    }
    // 读取 item 列表
    quint32 itemCount = 0;
    in >> itemCount;
    DA::DAChart3DItemSerialize serializer;
    for (quint32 i = 0; i < itemCount; ++i) {
        QByteArray itemData;
        in >> itemData;
        Qwt3DPlotItem* item = serializer.serializeIn(itemData);
        if (item) {
            item->attach(plot);
        }
    }
    return in;
}
