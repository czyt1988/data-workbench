#include "DATableExporter.h"
#include "DATableMetadata.h"
#include "DATableStyleManager.h"
#include "DATableConditionalFormatStore.h"
#include "DATableConditionalFormatRule.h"
#include "DATableNoteStore.h"
#include "DATableCellStyle.h"
#include "DATableDisplayFormat.h"
#include "DATableDataSource.h"
#include "DATableDataBlock.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "pandas/DAPyDataFrame.h"
#include <QColor>
#include <QCoreApplication>
#include <QFont>
#include <QIODevice>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QVector>
#include <algorithm>

namespace DA
{
namespace
{
/**
 * @brief HTML 文本转义（元素内容与属性值通用）
 */
QString htmlEscape(const QString& s)
{
    QString r;
    r.reserve(s.size());
    for (const QChar& ch : s) {
        switch (ch.unicode()) {
        case '&':
            r += QStringLiteral("&amp;");
            break;
        case '<':
            r += QStringLiteral("&lt;");
            break;
        case '>':
            r += QStringLiteral("&gt;");
            break;
        case '"':
            r += QStringLiteral("&quot;");
            break;
        case '\'':
            r += QStringLiteral("&#39;");
            break;
        default:
            r += ch;
            break;
        }
    }
    return r;
}

/**
 * @brief Markdown 表格单元格转义：竖线会破坏列结构，换行破坏行结构
 */
QString markdownEscape(const QString& s)
{
    QString r;
    r.reserve(s.size());
    for (const QChar& ch : s) {
        switch (ch.unicode()) {
        case '|':
            r += QStringLiteral("\\|");
            break;
        case '\n':
            r += QStringLiteral("<br>");
            break;
        case '\r':
            break;
        default:
            r += ch;
            break;
        }
    }
    return r;
}

/**
 * @brief 颜色转 CSS 色值（#rrggbb；带 alpha 时 rgba(...)）
 */
QString cssColor(const QColor& c)
{
    if (c.alpha() >= 255) {
        return QStringLiteral("#%1%2%3").arg(c.red(), 2, 16, QChar('0')).arg(c.green(), 2, 16, QChar('0')).arg(c.blue(), 2, 16, QChar('0'));
    }
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alpha() / 255.0, 0, 'f', 2);
}

/**
 * @brief 单元格装饰 → CSS 声明串（空串 = 无样式）
 *
 * 数据条以 CSS 线性渐变近似 delegate 的画法（条长 = (v-min)/(max-min) 归一化，
 * 沿条长方向 alpha 110→255 递增渐变，起点透明度 110/255≈0.43）。
 */
QString htmlCellStyleDeclarations(const DATableCellStyle& style, bool hasBar, double barRatio, const QColor& barColor)
{
    QStringList decls;
    if (style.backgroundValid() && style.background().color().alpha() > 0) {
        decls << QStringLiteral("background-color:") + cssColor(style.background().color());
    }
    if (hasBar) {
        const QColor c   = barColor.isValid() ? barColor : QColor(0xBD, 0xCD, 0xE8);
        const double pct = qBound(0.0, barRatio, 1.0) * 100.0;
        decls << QStringLiteral("background-image:linear-gradient(to right,rgba(%1,%2,%3,0.43) 0%,rgba(%1,%2,%3,1) %4%,rgba(0,0,0,0) %4%)")
                     .arg(c.red())
                     .arg(c.green())
                     .arg(c.blue())
                     .arg(pct, 0, 'f', 1);
    }
    if (style.foregroundValid()) {
        decls << QStringLiteral("color:") + cssColor(style.foreground());
    }
    if (style.fontValid()) {
        const QFont f = style.font();
        if (f.weight() >= QFont::Bold) {
            decls << QStringLiteral("font-weight:bold");
        }
        if (f.italic()) {
            decls << QStringLiteral("font-style:italic");
        }
        QStringList deco;
        if (f.underline()) {
            deco << QStringLiteral("underline");
        }
        if (f.strikeOut()) {
            deco << QStringLiteral("line-through");
        }
        if (!deco.isEmpty()) {
            decls << QStringLiteral("text-decoration:") + deco.join(QLatin1Char(' '));
        }
        decls << QStringLiteral("font-family:\"%1\"").arg(htmlEscape(f.family()));
        // pointSizeF() < 0 表示按像素指定，浏览器换算口径不同，跳过
        if (f.pointSizeF() > 0) {
            decls << QStringLiteral("font-size:%1pt").arg(f.pointSizeF(), 0, 'f', 1);
        }
    }
    return decls.isEmpty() ? QString() : decls.join(QLatin1Char(';'));
}
}  // namespace

/**
 * @brief 表格导出器私有数据
 *
 * 取数统一走 DATableDataSource::fetchBlock（与 DADataTableModel 的块缓存路径一致，
 * Python dataframe 与惰性源都是该接口的实现），按块取数不整表驻留。
 * 样式/显示格式/条件格式/批注均借自 DATableMetadata 束（只读借用，可空）。
 */
class DATableExporter::PrivateData
{
public:
    PrivateData(const DAData& d, const DATableMetadata* meta);

    // 分块取数（块边界钳制行数），成功填充 outRows（每元素为一行）
    bool fetchRows(qint64 start, int count, QList< QVariantList >& outRows, QString* err) const;
    // 值→显示文本（列显示格式优先，否则 QVariant 原样 toString，与模型 DisplayRole 同口径）
    QString displayText(int col, const QVariant& raw) const;
    // 条件格式求值上下文（按列惰性解析：规则指针 + 列统计/重复键预备计算）
    struct CfContext
    {
        bool resolved { false };
        const QVector< DATableConditionalFormatRule >* rules { nullptr };
        bool statsValid { false };
        DATableConditionalFormatStore::ColumnStats stats;
        QSet< QString > dupKeys;
        bool dupValid { false };
    };
    // col 越界（无元数据/列号异常）时返回的共享空上下文（无规则，调用方自然跳过求值）
    static CfContext& emptyCfContext()
    {
        static CfContext ctx;  // 函数内静态：首次调用构造，进程生命周期内共用
        return ctx;
    }
    CfContext& cfContext(int col);
    // 单元格装饰一次求值（层序与 DATableStyleItemDelegate::paint 一致）：
    // ①静态样式（三层 merge）← ②条件格式叠加；③瞬态高亮不导出；④数据条单独返回
    void cellDecorations(int row, int col, const QVariant& raw,
                         DATableCellStyle& style,
                         bool& hasBar,
                         double& barRatio,
                         QColor& barColor);
    // 共用渲染核心（复制与导出同一路径）：表头 + 按块数据行 + 尾部
    bool render(Format fmt, QTextStream& out, QString* err, const ProgressCallback& progress, const CancelCallback& cancel);

    DAData data;
    // dataframe 数据体（列统计/重复键求值用）：按需获取——DAPyDataFrame 默认构造会
    // import pandas（解释器未初始化时崩溃），成员只能用指针惰性构造
    std::unique_ptr< DAPyDataFrame > df;
    DATableDataSource* ts { nullptr };   ///< 取数源（fetchBlock；与 DADataTableModel 同口径）
    DATableStyleManager* styleManager { nullptr };
    DATableConditionalFormatStore* cfStore { nullptr };
    DATableNoteStore* noteStore { nullptr };
    qint64 rows { 0 };
    qint64 cols { 0 };
    QStringList columnNames;
    QVector< DATableDisplayFormat > colFormats;  ///< 列显示格式缓存（invalid = 未设置）
    QVector< CfContext > cfContexts;              ///< 条件格式求值上下文（按列惰性填充）
    bool valid { false };
};

/**
 * @brief 构造即解析数据与元数据（失败时 rows/cols 保持 0、valid=false）
 */
DATableExporter::PrivateData::PrivateData(const DAData& d, const DATableMetadata* meta) : data(d)
{
    // 判定口径：isTable()（DATableDataSource 非空）——覆盖 Python dataframe 与
    // 惰性表格源两条路径；series/数据包等非表格型拒绝
    if (!data.isTable()) {
        return;
    }
    // 成员为非 const DAData，取非 const tableSource()（fetchBlock 非 const 接口）
    ts = data.tableSource();
    if (!ts) {
        return;
    }
    rows = static_cast< qint64 >(ts->tableRowCount());
    cols = static_cast< qint64 >(ts->tableColumnCount());
    if (cols > 0) {
        columnNames.reserve(static_cast< int >(cols));
        for (qint64 c = 0; c < cols; ++c) {
            columnNames.append(ts->tableColumnName(static_cast< std::size_t >(c)));
        }
    }
    // 列统计/重复键求值用（dataframe 数据体）：仅当「有条件格式且为 Python
    // dataframe 且解释器已初始化」时才获取——DAPyDataFrame 默认构造会 import
    // pandas（解释器未初始化时崩溃）；惰性源（isDataFrame 为假）不获取，
    // 色阶/数据条/重复/唯一规则按求值器语义跳过（与 delegate 在惰性源上的表现一致）
    if (meta && meta->conditionalFormat() && !meta->conditionalFormat()->isEmpty() && data.isDataFrame()
        && DAPyScripts::isInitScripts()) {
        df.reset(new DAPyDataFrame(data.toDataFrame()));
    }
    if (meta) {
        // 元数据束只读借用（导出不改任何子存储）
        styleManager = meta->styleManager();
        cfStore      = meta->conditionalFormat();
        noteStore    = meta->notes();
        colFormats.resize(static_cast< int >(cols));
        for (qint64 c = 0; c < cols; ++c) {
            if (styleManager->hasColumnFormat(static_cast< int >(c))) {
                colFormats[ static_cast< int >(c) ] = styleManager->getColumnFormat(static_cast< int >(c));
            }
        }
        cfContexts.resize(static_cast< int >(cols));
    }
    valid = true;
}

/**
 * @brief 分块取数：DATableDataSource::fetchBlock（Python dataframe 与惰性源同接口）
 */
bool DATableExporter::PrivateData::fetchRows(qint64 start, int count, QList< QVariantList >& outRows, QString* err) const
{
    outRows.clear();
    if (count <= 0) {
        return true;
    }
    const DATableDataBlock block = ts->fetchBlock(static_cast< std::size_t >(start), static_cast< std::size_t >(count));
    // count 已按剩余行数钳制，请求行数 >=1 时空块即取数失败
    if (!block.isValid() || block.isEmpty()) {
        if (err) {
            *err = QCoreApplication::translate("DATableExporter", "Failed to read data block from row %1").arg(start + 1);  // cn:从第 %1 行读取数据块失败
        }
        return false;
    }
    outRows = QList< QVariantList >(block.cells().cbegin(), block.cells().cend());
    return true;
}

/**
 * @brief 值→显示文本：列显示格式优先（DATableDisplayFormat::formatValue），
 *        否则 QVariant 原样 toString —— 与 DADataTableModel::actualData 的
 *        DisplayRole 口径一致（表格内选区复制读的就是这条路径）
 */
QString DATableExporter::PrivateData::displayText(int col, const QVariant& raw) const
{
    if (col >= 0 && col < colFormats.size() && colFormats[ col ].isValid()) {
        return colFormats[ col ].formatValue(raw);
    }
    return raw.toString();
}

/**
 * @brief 条件格式求值上下文（按列惰性解析一次）
 *
 * 列统计/重复键与 delegate 走同一 Python 算子（da_column_stats / 列重复值）：
 * 页面未打开时 widget 装的 provider 已被清空，此处自算一次（每列至多一次
 * Python 调用）。惰性源（无 dataframe 数据体）两项均取不到——色阶/数据条/
 * 重复/唯一规则按求值器语义跳过，与 delegate 在惰性源上的表现一致。
 */
DATableExporter::PrivateData::CfContext& DATableExporter::PrivateData::cfContext(int col)
{
    // 无元数据（meta 为空）或列号越界：返回共享空上下文（无规则，调用方自然跳过求值）
    if (col < 0 || col >= cfContexts.size()) {
        return emptyCfContext();
    }
    CfContext& ctx = cfContexts[ col ];
    if (!ctx.resolved) {
        ctx.resolved = true;
        if (cfStore && col >= 0 && col < cfContexts.size()) {
            ctx.rules = cfStore->rulesForColumn(col);
            if (ctx.rules && !ctx.rules->isEmpty() && df && !df->isNone() && DAPyScripts::isInitScripts()) {
                DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
                if (cfStore->needsColumnStats(col)) {
                    const DAPyScriptsDataFrame::ColumnStatsResult r = py.columnStats(*df, col);
                    ctx.statsValid        = r.ok;
                    ctx.stats.minValue    = r.minValue;
                    ctx.stats.maxValue    = r.maxValue;
                    ctx.stats.medianValue = r.medianValue;
                }
                if (cfStore->needsColumnDuplicates(col)) {
                    QList< QVariant > values;
                    if (py.columnDuplicateValues(*df, col, values)) {
                        for (const QVariant& v : std::as_const(values)) {
                            const QString key = DATableConditionalFormatRule::duplicateKey(v);
                            if (!key.isEmpty()) {
                                ctx.dupKeys.insert(key);
                            }
                        }
                        ctx.dupValid = true;
                    }
                }
            }
        }
    }
    return ctx;
}

/**
 * @brief 单元格装饰一次求值：① 静态样式 ← ② 条件格式叠加；④ 数据条单独返回
 */
void DATableExporter::PrivateData::cellDecorations(int row, int col, const QVariant& raw,
                                                    DATableCellStyle& style,
                                                    bool& hasBar,
                                                    double& barRatio,
                                                    QColor& barColor)
{
    hasBar   = false;
    barRatio = 0.0;
    if (styleManager) {
        style = styleManager->resolveCellStyle(row, col);
    } else {
        style = DATableCellStyle();
    }
    CfContext& ctx = cfContext(col);
    if (ctx.rules && !ctx.rules->isEmpty()) {
        const DATableConditionalFormatStore::EvaluateResult r = DATableConditionalFormatStore::evaluate(
            *ctx.rules, raw, ctx.statsValid, ctx.stats, ctx.dupValid ? &ctx.dupKeys : nullptr);
        style.mergeFrom(r.style);
        hasBar   = r.hasDataBar;
        barRatio = r.barRatio;
        barColor = r.barColor;
    }
}

/**
 * @brief 共用渲染核心：表头 + 按块数据行 + 尾部（复制与导出同一路径）
 *
 * 每块取 c_blockRows 行（一次 fetchBlock），逐行渲染写入 out，内存占用与表
 * 大小无关；progress 按块边界回调，cancel 在块边界生效。
 * 无列表（cols==0）三种格式都输出空内容（HTML 亦不输出文档骨架）。
 * 表头行含列名（整个表复制自描述，粘贴到 Excel/Markdown 时带列名），
 * 不含索引列（与表格内选区复制同口径）。
 */
bool DATableExporter::PrivateData::render(Format fmt, QTextStream& out, QString* err,
                                          const ProgressCallback& progress, const CancelCallback& cancel)
{
    if (cols <= 0) {
        return true;
    }
    const int colCount = static_cast< int >(cols);
    // —— 表头 ——
    switch (fmt) {
    case Format::TabText:
        for (int c = 0; c < colCount; ++c) {
            if (c > 0) {
                out << QLatin1Char('\t');
            }
            out << columnNames[ c ];
        }
        out << QLatin1Char('\n');
        break;
    case Format::Markdown:
        out << QLatin1Char('|');
        for (int c = 0; c < colCount; ++c) {
            out << ' ' << markdownEscape(columnNames[ c ]) << " |";
        }
        out << QLatin1Char('\n') << QLatin1Char('|');
        for (int c = 0; c < colCount; ++c) {
            out << " --- |";
        }
        out << QLatin1Char('\n');
        break;
    case Format::Html: {
        out << "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<title>"
            << htmlEscape(data.getName()) << "</title>\n<style>\n"
            << "table.da-table{border-collapse:collapse;font-family:system-ui,'Microsoft YaHei','Segoe UI',sans-serif;font-size:10.5pt;}\n"
            << "table.da-table th,table.da-table td{border:1px solid #c8c8c8;padding:2px 6px;text-align:left;vertical-align:top;}\n"
            << "table.da-table thead th{background-color:#f3f3f3;}\n"
            << "</style>\n</head>\n<body>\n<table class=\"da-table\">\n<thead>\n<tr>";
        for (int c = 0; c < colCount; ++c) {
            out << "<th>" << htmlEscape(columnNames[ c ]) << "</th>";
        }
        out << "</tr>\n</thead>\n<tbody>\n";
        break;
    }
    }
    // —— 数据行按块迭代 ——
    QList< QVariantList > blockRows;
    qint64 done = 0;
    for (qint64 rowStart = 0; rowStart < rows; rowStart += c_blockRows) {
        if (cancel && cancel()) {
            if (err) {
                *err = QCoreApplication::translate("DATableExporter", "Operation canceled");  // cn:操作已取消
            }
            return false;
        }
        const int count = static_cast< int >(std::min< qint64 >(c_blockRows, rows - rowStart));
        if (!fetchRows(rowStart, count, blockRows, err)) {
            return false;
        }
        const int blockRowCount = blockRows.size();
        for (int i = 0; i < blockRowCount; ++i) {
            const QVariantList& row = blockRows.at(i);
            switch (fmt) {
            case Format::TabText:
                for (int c = 0; c < colCount; ++c) {
                    if (c > 0) {
                        out << QLatin1Char('\t');
                    }
                    out << displayText(c, c < row.size() ? row.at(c) : QVariant());
                }
                out << QLatin1Char('\n');
                break;
            case Format::Markdown:
                out << QLatin1Char('|');
                for (int c = 0; c < colCount; ++c) {
                    out << ' ' << markdownEscape(displayText(c, c < row.size() ? row.at(c) : QVariant())) << " |";
                }
                out << QLatin1Char('\n');
                break;
            case Format::Html: {
                const int actualRow = static_cast< int >(rowStart) + i;
                out << "<tr>";
                for (int c = 0; c < colCount; ++c) {
                    const QVariant raw = (c < row.size()) ? row.at(c) : QVariant();
                    DATableCellStyle style;
                    bool hasBar    = false;
                    double barRatio = 0.0;
                    QColor barColor;
                    cellDecorations(actualRow, c, raw, style, hasBar, barRatio, barColor);
                    const QString styleDecl = htmlCellStyleDeclarations(style, hasBar, barRatio, barColor);
                    // 批注以悬停提示携带（层⑤角标在 HTML 无对应物）
                    QString noteTitle;
                    if (noteStore && noteStore->hasNote(actualRow, c)) {
                        noteTitle = noteStore->noteText(actualRow, c);
                    }
                    out << "<td";
                    if (!styleDecl.isEmpty()) {
                        out << " style=\"" << styleDecl << "\"";
                    }
                    if (!noteTitle.isEmpty()) {
                        out << " title=\"" << htmlEscape(noteTitle) << "\"";
                    }
                    out << ">" << htmlEscape(displayText(c, raw)) << "</td>";
                }
                out << "</tr>\n";
                break;
            }
            }
        }
        done += blockRowCount;
        if (progress) {
            progress(done, rows);
        }
    }
    // —— 尾部 ——
    if (fmt == Format::Html) {
        out << "</tbody>\n</table>\n</body>\n</html>\n";
    }
    return true;
}

//===============================================================
// DATableExporter
//===============================================================

/**
 * @brief rows*cols 是否超过复制确认阈值
 */
bool DATableExporter::isLargeForCopy(qint64 rows, qint64 cols)
{
    return (rows > 0) && (cols > 0) && (rows * cols > c_largeCopyCellLimit);
}

/**
 * @brief 构造导出器（构造中解析数据与元数据，失败时 isValid()=false）
 */
DATableExporter::DATableExporter(const DAData& d, const DATableMetadata* meta)
    : d_ptr(std::make_unique< PrivateData >(d, meta))
{
}

DATableExporter::~DATableExporter()
{
}

/**
 * @brief 数据是否可导出（非 DataFrame 型/无取数源返回 false）
 */
bool DATableExporter::isValid(QString* why) const
{
    if (d_ptr->valid) {
        return true;
    }
    if (why) {
        *why = QCoreApplication::translate("DATableExporter", "Only table-shaped data can be exported");  // cn:仅支持导出表格型数据
    }
    return false;
}

qint64 DATableExporter::rowCount() const
{
    return d_ptr->rows;
}

qint64 DATableExporter::columnCount() const
{
    return d_ptr->cols;
}

/**
 * @brief 总单元格数（rows*cols）
 */
qint64 DATableExporter::cellCount() const
{
    return d_ptr->rows * d_ptr->cols;
}

/**
 * @brief 全量构建字符串（剪贴板复制路径）
 *
 * 按块取数逐块渲染拼接（与 write 共用同一 render 核心），不整表驻留取数，
 * 但字符串本身与表大小成正比——调用方须先做大小确认（isLargeForCopy）。
 */
bool DATableExporter::toString(Format fmt, QString* out, QString* err,
                               const ProgressCallback& progress, const CancelCallback& cancel) const
{
    if (!out) {
        return false;
    }
    out->clear();
    if (!isValid(err)) {
        return false;
    }
    QTextStream stream(out);
    const bool ok = d_ptr->render(fmt, stream, err, progress, cancel);
    stream.flush();
    return ok;
}

/**
 * @brief 增量写入设备（文件导出路径）
 *
 * dev 由调用方打开（写模式）；输出 UTF-8 文本（Qt5 显式 setCodec）。
 * 中途取消/失败时已写入的部分保留，调用方决定是否删除半成品文件。
 */
bool DATableExporter::write(Format fmt, QIODevice* dev, QString* err,
                            const ProgressCallback& progress, const CancelCallback& cancel) const
{
    if (!dev || !dev->isWritable()) {
        if (err) {
            *err = QCoreApplication::translate("DATableExporter", "Output device is not open for writing");  // cn:输出设备未以写模式打开
        }
        return false;
    }
    if (!isValid(err)) {
        return false;
    }
    QTextStream stream(dev);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#endif
    const bool ok = d_ptr->render(fmt, stream, err, progress, cancel);
    stream.flush();
    return ok;
}

}  // end of namespace DA
