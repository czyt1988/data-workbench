#ifndef DADATAGLOBAL_H
#define DADATAGLOBAL_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DADATA_BUILDLIB)
#define DADATA_API Q_DECL_EXPORT
#else
#define DADATA_API Q_DECL_IMPORT
#endif

#endif  // DAPYDATAANALYSISGLOBAL_H
