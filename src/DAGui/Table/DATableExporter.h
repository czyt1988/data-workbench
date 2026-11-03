#ifndef DATABLEEXPORTER_H
#define DATABLEEXPORTER_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include <QList>
#include <QString>
#include <functional>

class QIODevice;

namespace DA
{
class DATableMetadata;

/**
 * @brief 表格导出器：把表格型数据（Python DataFrame / 惰性表格数据源）转换为三种文本形态
 *
 * 复制（剪贴板）与导出（文件）共用同一取数与渲染核心：
 * - 判定口径 isTable()（DATableDataSource 非空）——覆盖 Python dataframe 与惰性源；
 * - 取数分块进行（c_blockRows 行/块，统一走 DATableDataSource::fetchBlock——
 *   Python dataframe 与惰性源都是该接口的实现），不整表驻留内存——
 *   文件导出路径配合 QTextStream 增量写入，任意大表内存占用恒定；
 * - 值→文本口径与表格模型 DisplayRole 一致：列显示格式优先，否则 QVariant 原样 toString；
 * - Tab 文本与表格内选区复制（DADataTableView::copySelectionToClipboard）同口径：
 *   行内 Tab 分隔、行间换行（含末行换行）；
 * - HTML 带内联样式，层序与 DATableStyleItemDelegate 绘制管线一致：
 *   ① DATableStyleManager 三层静态样式 → ② 条件格式（DATableConditionalFormatStore::evaluate
 *   静态求值，列统计/重复键由本类预备计算）→ ④ 数据条以 CSS linear-gradient 近似；
 *   ③ 瞬态高亮（查找/校验）是会话态不导出；批注以 td 的 title 属性携带。
 *
 * 非 QObject 值语义类；进度/取消经回调注入（复制/导出路径共用一套 UI 反馈）。
 */
class DAGUI_API DATableExporter
{
    DA_DECLARE_PRIVATE(DATableExporter)
public:
    /**
     * @brief 输出格式
     */
    enum class Format
    {
        TabText,   ///< Tab 分隔纯文本（与表格选区复制同口径）
        Markdown,  ///< Markdown 表格
        Html       ///< HTML 文档（含样式）
    };

    // 进度回调（processedRows 已处理行数，totalRows 总行数）
    using ProgressCallback = std::function< void(qint64, qint64) >;
    // 取消谓词：返回 true 时中止导出（块边界检查）
    using CancelCallback = std::function< bool() >;

    // 复制前的大小确认阈值（单元格数）：超过则提醒用户（复制需整表构建字符串）
    static constexpr qint64 c_largeCopyCellLimit = 1000000;
    // 单块取数行数
    static constexpr int c_blockRows = 256;

    // rows*cols 是否超过复制确认阈值
    static bool isLargeForCopy(qint64 rows, qint64 cols);

    /**
     * @brief 构造导出器（整表）
     * @param d 数据（须为表格型 isTable()；Python DataFrame 或惰性表格数据源）
     * @param meta 表元数据束（可空：无则跳过显示格式与样式/条件格式渲染）
     */
    DATableExporter(const DAData& d, const DATableMetadata* meta = nullptr);
    /**
     * @brief 构造导出器（列子集；列头右键「按选中列复制/导出」入口）
     * @param d 数据（须为表格型 isTable()）
     * @param meta 表元数据束（可空）
     * @param columns 要导出的数据列号（数据源列位置索引；空/全无效时回退整表）
     * @note 列序按传入顺序渲染（调用方给「视图顺序」即所见即所得）；
     *       样式/显示格式/条件格式/批注仍按原始列号解析，子集内外一致
     */
    DATableExporter(const DAData& d, const DATableMetadata* meta, const QList< int >& columns);
    ~DATableExporter();

    // 数据是否有效（非 DataFrame 型/无取数源返回 false；why 出参给用户可读原因）
    bool isValid(QString* why = nullptr) const;

    qint64 rowCount() const;
    qint64 columnCount() const;
    // 总单元格数（rows*cols，0 行表按 0 计）
    qint64 cellCount() const;

    // 全量构建字符串（剪贴板复制路径）：按块取数逐块拼接，不整表取数
    // 取消返回 false（err = 用户可读原因）；progress 按块边界回调
    bool toString(Format fmt, QString* out, QString* err = nullptr,
                  const ProgressCallback& progress = nullptr, const CancelCallback& cancel = nullptr) const;

    // 增量写入设备（文件导出路径）：dev 须已打开，本类负责 flush 不负责关闭
    bool write(Format fmt, QIODevice* dev, QString* err = nullptr,
               const ProgressCallback& progress = nullptr, const CancelCallback& cancel = nullptr) const;
};

}  // end of namespace DA
#endif  // DATABLEEXPORTER_H
