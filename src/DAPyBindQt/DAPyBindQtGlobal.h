#ifndef DAPYBINDQTGLOBAL_H
#define DAPYBINDQTGLOBAL_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAPYBINDQT_BUILDLIB)
#define DAPYBINDQT_API Q_DECL_EXPORT
#else
#define DAPYBINDQT_API Q_DECL_IMPORT
#endif

#endif  // DAPYDATAANALYSISGLOBAL_H
