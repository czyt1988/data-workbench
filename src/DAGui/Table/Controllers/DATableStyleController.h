#ifndef DATABLESTYLECONTROLLER_H
#define DATABLESTYLECONTROLLER_H
#include "DAGuiAPI.h"
#include "Table/Metadata/DATableCellStyle.h"
#include "Table/Metadata/DATableDisplayFormat.h"
#include <QList>

namespace DA
{
class DATablePageContext;
class DATableSelectionService;

/**
 * @brief 样式与列显示格式控制器（008 期3）
 *
 * 样式与显示格式的读/写/清除；选区层级判定（整列→列级，整行→行级，
 * 否则单元格级）；选中区代表样式/格式的逐属性一致性计算（全选卡顿
 * 修复的三级策略随迁）。构造注入上下文与选区服务，不持窗口指针。
 * 样式不写 DataFrame（红线3）：一切写操作经命令入页撤销栈（D4）。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableStyleController : public QObject
{
    Q_OBJECT
public:
    explicit DATableStyleController(const DATablePageContext& ctx,
                                   const DATableSelectionService& sel,
                                   QObject* parent = nullptr);

public:
    // 合并样式片段到选中区（fragment 只设部分属性，merge 到目标已有样式；自动判定层级）
    void mergeStyleToSelection(const DATableCellStyle& fragment);
    // 外框线：对选中区包围矩形四边应用边框（单元格选区=逐格外边；整列选区=列级左右+
    // 首末数据行上下；整行选区=行级上下+首末列左右），单条命令一次撤销
    void applyBorderOutlineToSelection();
    // 设置完整样式到选中区（整体替换，为格式刷准备；自动判定层级）
    void applyStyleToSelection(const DATableCellStyle& style);
    // 清除选中区样式（自动判定层级，仅清除有样式的目标；无样式不推空命令）
    void clearStyleSelection();
    // 清除整表所有样式（可撤销）
    void clearStyleAll();
    // 获取选中区代表的样式（各属性独立判断一致性，不一致的属性 valid=false；
    // 全选卡顿修复的三级策略：列级快路径/小选区逐格/大选区返回空样式）
    DATableCellStyle getCurrentCellStyle() const;
    // 设置列显示格式到选中列（列级，可撤销）
    void setDisplayFormatToSelection(const DATableDisplayFormat& fmt);
    // 清除选中列的显示格式（列级，保留 bg/fg/font，可撤销）
    void clearDisplayFormatFromSelection();
    // 获取选中列代表的显示格式（不一致返回 invalid）
    DATableDisplayFormat getCurrentColumnDisplayFormat() const;

private:
    // 收集当前选中区涉及的列号（列级显示格式操作用）：
    // 优先整列选中，否则取选中单元格涉及列；都空返回空列表
    QList< int > collectSelectedColumns(bool ensureInDataframe) const;

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
};
}  // end of namespace DA
#endif  // DATABLESTYLECONTROLLER_H
