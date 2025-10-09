#ifndef DAGUIAPI_H
#define DAGUIAPI_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAGUI_BUILDLIB)
#define DAGUI_API Q_DECL_EXPORT
#else
#ifdef Q_CC_MSVC
#define DAGUI_API Q_DECL_IMPORT
#else
#define DAGUI_API Q_DECL_IMPORT
#endif
#endif

namespace DA
{
/**
 * @brief DAGui 模块进行meta类型初始化
 *
 * 此函数的实现在DAGuiEnumStringUtils.cpp
 */
void DAGUI_API da_gui_register_metatypes();
}
#endif  // DAGUIAPI_H
