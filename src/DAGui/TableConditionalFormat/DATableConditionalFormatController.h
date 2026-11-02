#ifndef DATABLECONDITIONALFORMATCONTROLLER_H
#define DATABLECONDITIONALFORMATCONTROLLER_H
#include "DAGuiAPI.h"
#include "DATableConditionalFormatRule.h"
#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QVector>

class QMenu;
class QUndoCommand;

namespace DA
{
class DATablePageContext;
class DATableSelectionService;

/**
 * @brief 条件格式控制器（008 期3）
 *
 * 规则编辑对话框、WPS 式预设菜单构造、作用域解析、预设一键施加、
 * 规则集差异→命令（增/删/改/移父命令）。构造注入上下文与选区服务，
 * 不持窗口指针。条件格式不写 DataFrame（红线3）：一切写操作经差异
 * 生成命令入页撤销栈（D4）。色阶/数据条需要全列统计，惰性/只读源上置灰。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableConditionalFormatController : public QObject
{
    Q_OBJECT
public:
    explicit DATableConditionalFormatController(const DATablePageContext& ctx,
                                               const DATableSelectionService& sel,
                                               QObject* parent = nullptr);

public:
    // 打开规则编辑对话框（initialType 为 DATableConditionalFormatRule::Type 整数值，
    // -1=不预设；菜单「其他规则」入口传对应类型）；确定后按差异生成父命令入栈
    void showDialog(int initialType = -1);
    // 清除选中列的条件格式规则（一次命令，可撤销）
    void clearFromSelection();
    // 清除全表条件格式规则（一次命令，可撤销）
    void clearAll();
    // 填充条件格式菜单（WPS 式：数据条/色阶/突出显示单元格规则/清除规则/自定义设置；
    // 每次调用先 clear 再填充；Ribbon 与右键子菜单共用）
    void populateMenu(QMenu* menu);

private:
    // 条件格式入口作用列：选区覆盖列；无选区回退当前格所在列；空列表 = 无有效列
    QList< int > scopeColumns() const;
    // 给每个作用列追加一条规则（预设一键施加；差异生成命令入栈）
    void applyRuleToScope(const DATableConditionalFormatRule& rule);
    // 突出显示单元格规则/重复值快速对话框：确定后施加到作用列
    void showQuickCellRuleDialog(DATableConditionalFormatRule::Condition cond);
    // 规则集差异 → 撤销命令（父命令聚合，旧新对象记录范式 D4）；无差异返回 nullptr
    QUndoCommand* buildCommands(const QHash< int, QVector< DATableConditionalFormatRule > >& newRules) const;

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
};
}  // end of namespace DA
#endif  // DATABLECONDITIONALFORMATCONTROLLER_H
