#ifndef DAAXOFFICEWRAPPERGLOBAL_H
#define DAAXOFFICEWRAPPERGLOBAL_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"
class QAxObject;
#if defined(DAAXOFFICEWRAPPER_BUILDLIB)
#define DAAXOFFICEWRAPPER_API Q_DECL_EXPORT
#else
#ifdef Q_CC_MSVC
#define DAAXOFFICEWRAPPER_API Q_DECL_IMPORT
#else
#define DAAXOFFICEWRAPPER_API Q_DECL_IMPORT
#endif
#endif

namespace DA
{
/**
 * @brief 判断QAxObject指针是否为空，如果不为空，判断AxObject是否为空，两个之一为空返回true
 * @param obj
 * @return
 * @note 实现位于DAAxObjectWordWrapper.cpp
 */
bool qaxobject_is_null(QAxObject* obj);
}
#endif  // DAAXOFFICEWRAPPERGLOBAL_H
