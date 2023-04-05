#ifndef DAGLOBALS_H
#define DAGLOBALS_H
#include <QScopedPointer>
#include <QString>
///
/// \def 前置声明的定义
///
#ifndef DA_IMPL_FORWARD_DECL
#define DA_IMPL_FORWARD_DECL(ClassName)	\
    class ClassName ## Private;
#endif
///
/// \def 前置声明的定义(带命名空间)
///
#ifndef DA_IMPL_FORWARD_DECL_NS
#define DA_IMPL_FORWARD_DECL_NS(NS,ClassName)	\
    namespace NS{class ClassName ## Private;}
#endif
///
/// \def impl的简易实现
///
#ifndef DA_IMPL
#define DA_IMPL(Class)								 \
private:									 \
    inline Class ## Private *d_func() { return (d_ptr.data()); }		 \
    inline const Class ## Private *d_func() const { return (d_ptr.data()); } \
    friend class Class ## Private;						 \
    QScopedPointer<Class ## Private> d_ptr;
#endif
///
/// \def impl的定义
///
#ifndef DA_IMPL_PUBLIC
#define DA_IMPL_PUBLIC(Class)								   \
    inline Class *q_func() { return (static_cast<Class *>(q_ptr)); }		   \
    inline const Class *q_func() const { return (static_cast<const Class *>(q_ptr)); } \
    friend class Class;								   \
    Class *q_ptr;
#endif
///
/// \def impl获取指针，参考Q_D
///
#ifndef DA_D
#define DA_D(Class,pointerName) \
    Class ## Private *pointerName = d_func()
#endif
///
/// \def impl获取指针，参考Q_D
///
#ifndef DA_DC
#define DA_DC(Class,pointerName) \
    const Class ## Private *pointerName = d_func()
#endif
///
/// \def impl获取指针，参考Q_Q
///
#ifndef DA_Q
#define DA_Q(Class) \
    Class *q = q_func()
#endif
///
/// \def impl获取指针，参考Q_Q
///
#ifndef DA_QC
#define DA_QC(Class) \
    const Class *q = q_func()
#endif


#endif // GLOBALS_H
