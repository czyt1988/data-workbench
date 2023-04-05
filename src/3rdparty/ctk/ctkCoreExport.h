// .NAME __ctkCoreExport - manage Windows system differences
// .SECTION Description
// The __ctkCoreExport captures some system differences between Unix
// and Windows operating systems.

#ifndef __ctkCoreExport_h
#define __ctkCoreExport_h

#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
#if defined(MAKE_CTK_LIB)
#define CTK_CORE_EXPORT Q_DECL_EXPORT
#endif
#endif

#if !defined(CTK_CORE_EXPORT)
#if defined(CTK_SHARED)
#define CTK_CORE_EXPORT Q_DECL_EXPORT
#else
#define CTK_CORE_EXPORT Q_DECL_IMPORT
#endif
#endif
#endif
