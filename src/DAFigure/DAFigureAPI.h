#ifndef DAFIGUREAPI_H
#define DAFIGUREAPI_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAFIGURE_BUILD)
#define DAFIGURE_API Q_DECL_EXPORT
#else
#define DAFIGURE_API Q_DECL_IMPORT
#endif

#endif  // DAFIGUREAPI_H
