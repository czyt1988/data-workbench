#ifndef DAUTILSAPI_H
#define DAUTILSAPI_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAUTILS_BUILD)
#define DAUTILS_API Q_DECL_EXPORT
#else
#define DAUTILS_API Q_DECL_IMPORT
#endif

// Q_DECL_IMPORT.
#endif  // DAUTILSAPI_H
