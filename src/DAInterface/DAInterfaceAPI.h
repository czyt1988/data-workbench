#ifndef DAINTERFACEGLOBAL_H
#define DAINTERFACEGLOBAL_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAINTERFACE_BUILDLIB)
#define DAINTERFACE_API		Q_DECL_EXPORT
#else
#define DAINTERFACE_API		Q_DECL_IMPORT
#endif

#endif // DAINTERFACEGLOBAL_H
