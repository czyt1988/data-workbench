#ifndef DATABLESTYLEITEMDELEGATE_H
#define DATABLESTYLEITEMDELEGATE_H
#include "DAGuiAPI.h"
#include <QStyledItemDelegate>

namespace DA
{
class DATableStyleManager;
class DATableHighlightOverlay;
class DATableConditionalFormatStore;

/**
 * @brief 表格样式自定义 delegate（五层绘制管线）
 *
 * paint() 按固定层序绘制（设计稿 D2 装饰管线，层序钉死勿调换，红线8）：
 * ① 静态样式（DATableStyleManager 三层 merge）→ ② 持久装饰（条件格式规则求值，003）
 * → ③ 瞬态装饰（DATableHighlightOverlay 查找/校验高亮）→ ④ 数据条（003）
 * → ⑤ 文字（基类绘制，批注角标 005 接入）。
 *
 * 无装饰快速路径保留：样式管理器、条件格式、高亮层都为空时直接走父类 paint。
 * 行号一律翻译为数据源绝对行（getCacheWindowStartRow 偏移）后再查询样式、条件格式与高亮。
 *
 * 条件格式求值（②）只在 delegate 层做（红线1：不进 model/data()），取值 Qt::EditRole；
 * 每格只查该列的规则桶（红线2：store 按列分桶缓存，不遍历全部规则），
 * 色阶/数据条的列统计经 store 惰性计算 + 缓存（不每格重算）。
 */
class DAGUI_API DATableStyleItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    DATableStyleItemDelegate(DATableStyleManager* mgr, DATableHighlightOverlay* overlay = nullptr, QObject* parent = nullptr);

    // 注入条件格式子存储（②持久装饰 + ④数据条；非拥有，可空）
    void setConditionalFormatStore(DATableConditionalFormatStore* store);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    DATableStyleManager* mStyleManager;              // 非拥有
    DATableHighlightOverlay* mOverlay;               // 非拥有（瞬态高亮层，可空）
    DATableConditionalFormatStore* mCfStore { nullptr };  // 非拥有（条件格式子存储，可空）
};
}  // end of namespace DA
#endif  // DATABLESTYLEITEMDELEGATE_H
