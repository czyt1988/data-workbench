#ifndef BASEGLOBAL_H
#define BASEGLOBAL_H
#include <QtCore/QtGlobal>

#if defined(BASE_PLUGIN_BUILD)
#define BASE_API Q_DECL_EXPORT
#else
#define BASE_API Q_DECL_IMPORT
#endif

// Q_DECL_IMPORT.
#endif  // BASEGLOBAL_H
