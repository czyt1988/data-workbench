#ifndef DATABLEMETADATA_H
#define DATABLEMETADATA_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QDomDocument>
#include <QDomElement>
#include <QList>
#include <QPair>
#include <QString>

namespace DA
{
class DATableStyleManager;
class DATableViewStateStore;
class DATableConditionalFormatStore;
class DATableComputedColumnStore;

/**
 * @brief 一个数据集的表元数据束
 *
 * 会话级注册表 DATableMetadataRegistry 按 DAData 键控持有一份本对象，聚合该数据集
 * 的所有表级元数据子存储。当前含四个子存储：
 * - styleManager：行/列/单元格静态样式与列显示格式（既有能力）
 * - viewState：视图状态（列宽按列名 / 行头宽，R6）
 * - conditionalFormat：条件格式规则集（003 T1.5，渲染类，位置键列引用 R20）
 * - computedColumns：计算列定义集合（004 T1.23，语义类，定义类在 src/DAData，列名引用 R20）
 *
 * 批注 / 校验由后续计划（005）各自往束里加子存储与
 * XML 段，本类只提供聚合与统一序列化入口 toXml/fromXml。
 *
 * 归属分层（D3）：渲染类子存储（样式，后续含条件格式/批注）与语义类子存储
 * （视图状态按列名引用；后续校验/计算列的定义类放 src/DAData）都挂在本束下，
 * 存储与序列化统一走注册表 → 工程文件 table-metadata.xml。
 *
 * 生命周期：本对象与其子存储的 QObject parent 均为注册表；随数据存在，widget 关闭不销毁。
 */
class DAGUI_API DATableMetadata : public QObject
{
    Q_OBJECT
public:
    explicit DATableMetadata(QObject* parent = nullptr);
    ~DATableMetadata();

    // 样式管理器（随束创建，parent = this，非空）
    DATableStyleManager* styleManager() const;
    // 视图状态子存储（随束创建，parent = this，非空）
    DATableViewStateStore* viewState() const;
    // 条件格式子存储（随束创建，parent = this，非空）
    DATableConditionalFormatStore* conditionalFormat() const;
    // 计算列子存储（随束创建，parent = this，非空，004 T1.23）
    DATableComputedColumnStore* computedColumns() const;

    // 聚合序列化：把各子存储的 XML 段写入 table 元素
    void toXml(QDomDocument& doc, QDomElement& tableEle) const;
    // 聚合反序列化：从 table 元素读取各子存储的 XML 段（旧 table-styles.xml 无 <view-state>/
    // <conditional-format>/<computed-column> 段，自动跳过；各子存储 fromXml 先自清空，
    // 因此本函数也是整束恢复入口——就地形状变换的撤销还原用）
    bool fromXml(const QDomElement& tableEle);
    // 全部子存储均为空返回 true（保存时跳过空条目）
    bool isEmpty() const;
    // 清空全部子存储（004 T1.27 就地形状变换：行位置键与列名引用全部失去意义时整束清除）
    void clearAll();
    // 整束快照/恢复（序列化为 XML 文本；就地形状变换与元数据清理同一撤销单元用，
    // restore 等价 fromXml——各子存储先自清空再读入）
    QString snapshotToString() const;
    bool restoreFromString(const QString& snapshot);
    // 各类别条目数统计（就地替换确认框列出将被清除的元数据类别与条目数，004 T1.27）：
    // 返回 (类别显示名, 条目数) 列表，只含非空类别
    QList< QPair< QString, int > > entryCountsByCategory() const;

private:
    DATableStyleManager* mStyleManager { nullptr };    ///< 样式子存储（拥有，parent=this）
    DATableViewStateStore* mViewState { nullptr };     ///< 视图状态子存储（拥有，parent=this）
    DATableConditionalFormatStore* mConditionalFormat { nullptr };  ///< 条件格式子存储（拥有，parent=this）
    DATableComputedColumnStore* mComputedColumns { nullptr };       ///< 计算列子存储（拥有，parent=this，004 T1.23）
};
}  // end of namespace DA
#endif  // DATABLEMETADATA_H
