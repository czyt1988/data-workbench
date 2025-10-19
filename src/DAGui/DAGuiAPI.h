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

#endif  // DAGUIAPI_H
