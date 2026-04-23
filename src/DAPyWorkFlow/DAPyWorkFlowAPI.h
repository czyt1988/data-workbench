#ifndef DAPYWORKFLOWAPI_H
#define DAPYWORKFLOWAPI_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAPYWORKFLOW_BUILDLIB)
#define DAPYWORKFLOW_API Q_DECL_EXPORT
#else
#ifdef Q_CC_MSVC
#define DAPYWORKFLOW_API Q_DECL_IMPORT
#else
#define DAPYWORKFLOW_API Q_DECL_IMPORT
#endif
#endif

namespace DA
{

}  // namespace DA

#endif  // DAPYWORKFLOWAPI_H