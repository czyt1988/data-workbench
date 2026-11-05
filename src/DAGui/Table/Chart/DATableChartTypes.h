#ifndef DATABLECHARTTYPES_H
#define DATABLECHARTTYPES_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include <QList>
#include <QString>
#include <cstddef>

namespace DA
{

/**
 * @brief 表格列在快速绘图中的角色
 *
 * 由 DATableDataSource::tableColumnType 判定；类型不可静态判定时用少量样本值推断。
 */
enum class DATableColumnRole
{
    Unsupported = 0,  ///< 复杂对象/二进制/全空，无法画图
    Numeric,          ///< 数值列
    DateTime,         ///< 日期/时间列
    Categorical       ///< 字符串/对象/布尔/类别列
};

/**
 * @brief 快速绘图支持的图型（v3：12 种）
 *
 * 图型身份沿用 DA::DAChartTypes 的既有含义，这里只表达「浮窗可选图型」这一层选择，
 * 不新建一套图表类型系统。新增图型一律追加在尾部，避免改变已有枚举值。
 */
enum class DATableChartType
{
    None = 0,
    Line,         ///< 折线（单列 / 时间+数值 / 多序列）
    Scatter,      ///< 散点（两列数值）
    Histogram,    ///< 直方图（单列数值）
    Boxplot,      ///< 箱线图（每列一个箱 / 分类+数值按类别分箱）
    Bar,          ///< 分类柱状（分组聚合或类别计数）
    MultiLine,    ///< 多序列折线（3+ 数值列）
    Contour,      ///< 等高线（x/y/值 三列数值）
    ErrorBar,     ///< 误差棒（x/y/误差 三列数值）
    VectorField,  ///< 向量场（x/y/u/v 四列数值）
    MultiBar,     ///< 多重柱状（分类 + 2+ 数值列，按类别聚合分组展示）
    Surface3D,    ///< 3D 曲面（x/y/z 三列数值，仅散点插值模式）
    Line3D        ///< 3D 线图（x/y/z 三列数值）
};

/**
 * @brief 分类聚合口径
 */
enum class DATableChartAggregation
{
    Mean = 0,
    Median,
    Sum,
    Count,
    Std,
    Var,
    Min,
    Max
};

/**
 * @brief 请求中的一列（有序选区的成员）
 */
struct DAGUI_API DATableChartColumn
{
    int dataIndex { -1 };                        ///< 数据列号（pandas 列位置）
    QString name;                                ///< 列名
    int occurrence { 0 };                        ///< 重名列中的第几个（0 起）
    DATableColumnRole role { DATableColumnRole::Unsupported };  ///< 列角色

    bool isNumeric() const { return role == DATableColumnRole::Numeric; }
    bool isDateTime() const { return role == DATableColumnRole::DateTime; }
    bool isCategorical() const { return role == DATableColumnRole::Categorical; }
    bool isSupported() const { return role != DATableColumnRole::Unsupported; }
};

/**
 * @brief 表格侧构造的绘图请求（表格 → 图的唯一输入）
 *
 * 「数据集句柄 + 按视图顺序的有序选中列 + 角色 + 行数/惰性标记」。
 * 表格侧只负责构造它，取数与图元构建都在绘图侧完成。
 */
struct DAGUI_API DATableChartRequest
{
    DAData data;                    ///< 数据集句柄
    QList<DATableChartColumn> columns;  ///< 有序选中列（按视图顺序）
    /// 整表列信息（按数据列号顺序；设置区列下拉可换整表任意列）。
    /// 只填静态可判定的角色，避免为宽表逐列取样本；缺省时列下拉退回 columns。
    QList<DATableChartColumn> allColumns;
    std::size_t rowCount { 0 };     ///< 数据行数
    bool lazy { false };            ///< 数据源是否为惰性加载

    // 数据集是否可用（非表格型数据无法快速绘图）
    bool isValid() const;
    // 数值列个数
    int numericCount() const;
    // 分类列个数
    int categoricalCount() const;
    // 时间列个数
    int dateTimeCount() const;
    // 第一个指定角色的列在 columns 中的下标，没有返回 -1
    int indexOfFirst(DATableColumnRole role) const;
    // 返回指定角色在 columns 中的全部下标
    QList< int > indexesOf(DATableColumnRole role) const;
};

/**
 * @brief 绘图规格（图型 + X/Y 角色 + 聚合 + 排序 + Top N）
 *
 * 列一律用 request.columns 的下标表达，-1 表示不使用/行号。
 */
struct DAGUI_API DATableChartSpec
{
    DATableChartType type { DATableChartType::None };
    int xColumn { -1 };             ///< X 列下标；-1 表示行号
    int yColumn { -1 };             ///< Y 列下标（散点/等高线/误差棒/向量场/3D 的角色列）
    int zColumn { -1 };             ///< Z 列下标（等高线值 / 3D 曲面与 3D 线图的 Z）
    int errorColumn { -1 };         ///< 误差列下标（误差棒）
    int uColumn { -1 };             ///< U 列下标（向量场）
    int vColumn { -1 };             ///< V 列下标（向量场）
    QList< int > valueColumns;      ///< 数值列下标（折线/散点/箱线/多序列/多重柱状）
    int categoryColumn { -1 };      ///< 分类列下标（柱状/按类别箱线/多重柱状）
    DATableChartAggregation aggregation { DATableChartAggregation::Mean };
    int categoryTopN { 30 };        ///< 类别数超限时保留的数量（0 表示不截取）
    int histogramBins { 20 };       ///< 直方图分箱数
    bool sortByValueDesc { true };  ///< 分类柱状按聚合值降序（统计向导保持按类别名排序）

    bool isValid() const;
    // 深比较：切图型/重画时判断规格是否变化
    bool equals(const DATableChartSpec& other) const;
};

/**
 * @brief 无法出图的原因（界面侧负责翻译）
 */
enum class DATableChartRejectReason
{
    None = 0,
    EmptySelection,          ///< 没有选中列
    NotTableData,            ///< 数据集不是表格型
    NoNumericColumn,         ///< 选区没有数值列
    OnlyTimeColumn,          ///< 只有一列时间列
    UnsupportedCombination   ///< v1 不支持的列组合（如分类+2 数值、2 分类）
};

/**
 * @brief 规划结果：可选图型（首个为默认）+ 默认规格
 */
struct DAGUI_API DATableChartPlan
{
    QList< DATableChartType > types;  ///< 可选图型；首个即默认图型
    QList< DATableChartSpec > typeSpecs;  ///< 与 types 平行：每种图型在该选区下的默认规格
    DATableChartSpec defaultSpec;     ///< 默认图型的规格
    DATableChartRejectReason reject { DATableChartRejectReason::None };  ///< 无法出图的原因

    bool isPlottable() const { return reject == DATableChartRejectReason::None && !types.isEmpty(); }
};

/**
 * @brief 一次构建的附加信息（浮窗底部与正式图表角标使用）
 */
struct DAGUI_API DATableChartBuildInfo
{
    bool sampled { false };           ///< 是否对数据做了采样
    int sampledRows { 0 };            ///< 参与绘制的行数（采样后）
    int totalRows { 0 };              ///< 数据总行数
    int droppedMissing { 0 };         ///< 因缺失值被剔除的行数
    bool approximate { false };       ///< 聚合/分位数是否基于采样（近似）
    int displayedCategories { 0 };    ///< 实际显示的类别数
    int totalCategories { 0 };        ///< 聚合前的类别总数
    bool sampleLimited { false };     ///< 是否因预算上限停止采样
};

}  // namespace DA

#endif  // DATABLECHARTTYPES_H
