#ifndef DAPLOTAPI_H
#define DAPLOTAPI_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAPLOT_BUILD)
#define DAPLOT_API Q_DECL_EXPORT
#else
#define DAPLOT_API Q_DECL_IMPORT
#endif
namespace DA
{

enum class DAPlotTreeItemRole
{
    RoleItemType     = Qt::UserRole + 1,  // 存储节点类型
    RoleInnerPointer = Qt::UserRole + 2   // 存储内部指针
};

/**
 * @brief @ref DAPlotTreeItemRole 对应的识别
 */
enum class DAPlotTreeItemType
{
    Unknow = 1001,
    Figure,
    Plot,            ///< RoleInnerPointer可提取QImPlotNode指针
    AxesFolder,      ///< 存放坐标轴的文件夹
    Axis,            ///< RoleInnerPointer可提取QImPlotNode指针
    PlotItemFloder,  ///< 存放plotitem的文件夹
    PlotItem         ///< RoleInnerPointer可提取QImPlotItemNode指针
};
}  // end namespace DA
#endif  // DAFIGUREAPI_H
