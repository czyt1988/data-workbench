#ifndef DATABLEDATAOPERATION_H
#define DATABLEDATAOPERATION_H
#include "DAGuiAPI.h"
#include <QString>

namespace DA
{
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;

/**
 * @brief 表格数据操作接口（008 期4，Strategy）
 *
 * 一个数据操作一个文件：对话框入口（分列/合并/分组/整形/填充/类型转换…）实现
 * 本接口经 Registry 按 id 调用（runOperation("split-column")）；参数化入口
 * （插删行/列、粘贴等由菜单/右键携带参数的）由 Facade 直接调具体算子的具名方法。
 * 依赖：构造注入上下文、选区服务与编辑后效协调器；不持窗口指针。
 * 生命周期：宿主页面构造并持有（QObject 父子持有对话框）。
 */
class DAGUI_API DATableDataOperation
{
public:
    virtual ~DATableDataOperation() = default;
    // 操作标识（如 "split-column"；Registry 键，不翻译）
    virtual QString id() const = 0;
    // 操作标题（tr() 翻译；撤销栈文案/菜单文案共用）
    virtual QString title() const = 0;
    // 是否可用（选区/只读/惰性守卫；菜单启用规则与入口守卫共用）
    virtual bool isAvailable() const = 0;
};
}  // end of namespace DA
#endif  // DATABLEDATAOPERATION_H
