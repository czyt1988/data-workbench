#ifndef DATABLESTYLEITEMDELEGATE_H
#define DATABLESTYLEITEMDELEGATE_H
#include "DAGuiAPI.h"
#include <QStyledItemDelegate>

namespace DA
{
class DATableStyleManager;
class DATableHighlightOverlay;

/**
 * @brief 表格样式自定义 delegate（五层绘制管线）
 *
 * paint() 按固定层序绘制（设计稿 D2 装饰管线）：
 * ① 静态样式（DATableStyleManager 三层 merge）→ ② 持久装饰（条件格式，003 接入）
 * → ③ 瞬态装饰（DATableHighlightOverlay 查找/校验高亮）→ ④ 数据条（003 接入）
 * → ⑤ 文字（基类绘制，批注角标 005 接入）。
 *
 * 无装饰快速路径保留：样式管理器为空且高亮层无内容时直接走父类 paint。
 * 行号一律翻译为数据源绝对行（getCacheWindowStartRow 偏移）后再查询样式与高亮。
 */
class DAGUI_API DATableStyleItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    DATableStyleItemDelegate(DATableStyleManager* mgr, DATableHighlightOverlay* overlay = nullptr, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    DATableStyleManager* mStyleManager;  // 非拥有
    DATableHighlightOverlay* mOverlay;   // 非拥有（瞬态高亮层，可空）
};
}  // end of namespace DA
#endif  // DATABLESTYLEITEMDELEGATE_H
