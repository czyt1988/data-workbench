#ifndef DATABLESTYLEITEMDELEGATE_H
#define DATABLESTYLEITEMDELEGATE_H
#include "DAGuiAPI.h"
#include <QStyledItemDelegate>
#include <QBrush>
#include <QHash>
#include <QFont>
#include <QFontMetrics>

namespace DA
{
class DATableStyleManager;
class DATableHighlightOverlay;
class DATableConditionalFormatStore;
class DATableValidationStore;
class DATableNoteStore;

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
    // 注入校验规则子存储（枚举列下拉编辑器，005 T1.25；非拥有，可空）
    void setValidationStore(DATableValidationStore* store);
    // 注入批注子存储（层⑤之后的右上角角标 + helpEvent tooltip，005 T1.26；非拥有，可空）
    void setNoteStore(DATableNoteStore* store);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    // 创建编辑器（005 T1.25）：所在列有启用的枚举规则 → QComboBox（editable=
    // 「允许其他值」开关）；否则走基类默认。只读门禁沿用模型 flags（视图对不可编辑
    // 索引根本不进 createEditor，红线4：不另造一套可编辑判定）
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;
    // 悬停 tooltip（005 T1.26）：有批注的格显示批注文本（helpEvent 只对可见单元格
    // 触发，天然满足"只查可见单元格"）；无批注走基类
    bool helpEvent(QHelpEvent* event, QAbstractItemView* view,
                   const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    DATableStyleManager* mStyleManager;              // 非拥有
    DATableHighlightOverlay* mOverlay;               // 非拥有（瞬态高亮层，可空）
    DATableConditionalFormatStore* mCfStore { nullptr };  // 非拥有（条件格式子存储，可空）
    DATableValidationStore* mValidationStore { nullptr };  // 非拥有（校验规则子存储，可空，005）
    DATableNoteStore* mNoteStore { nullptr };              // 非拥有（批注子存储，可空，005 T1.26）
    // 数据条渐变画刷缓存（按颜色键；paint()是const方法，缓存成员必须mutable）
    mutable QHash< QRgb, QBrush > mDataBarBrushes;
    // 取数据条渐变画刷（单位坐标+对象边界模式，命中缓存直接返回）
    QBrush dataBarBrush(const QColor& barColor) const;
    // 自定义字体度量缓存（按字体键；字体种类通常很少，只按字体增长）
    mutable QHash< QFont, QFontMetrics > mFontMetricsCache;
    // 取字体度量（命中缓存直接返回；Qt5的QFontMetrics无默认构造，只用find/insert）
    QFontMetrics fontMetricsFor(const QFont& font) const;
};
}  // end of namespace DA
#endif  // DATABLESTYLEITEMDELEGATE_H
