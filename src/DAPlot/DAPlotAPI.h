#ifndef DAPLOTAPI_H
#define DAPLOTAPI_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAPLOT_BUILD)
#define DAPLOT_API Q_DECL_EXPORT
#else
#define DAPLOT_API Q_DECL_IMPORT
#endif


#endif  // DAFIGUREAPI_H
