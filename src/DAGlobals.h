#ifndef DAGLOBALS_H
#define DAGLOBALS_H
#include <QScopedPointer>
#include <QString>
#include "DAConfigs.h"
/**
 *@def DA_IMPL_FORWARD_DECL
 *@brief 前置声明的定义
 */
#ifndef DA_IMPL_FORWARD_DECL
#define DA_IMPL_FORWARD_DECL(ClassName) class ClassName##Private;
#endif

/**
 *@def DA_IMPL_FORWARD_DECL_NS
 *@brief 前置声明的定义(带命名空间)
 */
#ifndef DA_IMPL_FORWARD_DECL_NS
#define DA_IMPL_FORWARD_DECL_NS(NS, ClassName)                                                                         \
    namespace NS                                                                                                       \
    {                                                                                                                  \
    class ClassName##Private;                                                                                          \
    }
#endif

/**
 *@def DA_IMPL
 *@brief impl的简易实现
 */
#ifndef DA_IMPL
#define DA_IMPL(Class)                                                                                                 \
private:                                                                                                               \
    inline Class##Private* d_func()                                                                                    \
    {                                                                                                                  \
        return (d_ptr.data());                                                                                         \
    }                                                                                                                  \
    inline const Class##Private* d_func() const                                                                        \
    {                                                                                                                  \
        return (d_ptr.data());                                                                                         \
    }                                                                                                                  \
    friend class Class##Private;                                                                                       \
    QScopedPointer< Class##Private > d_ptr;
#endif

/**
 *@def DA_IMPL_PUBLIC
 *@brief impl的定义
 */
#ifndef DA_IMPL_PUBLIC
#define DA_IMPL_PUBLIC(Class)                                                                                          \
    inline Class* q_func()                                                                                             \
    {                                                                                                                  \
        return (static_cast< Class* >(q_ptr));                                                                         \
    }                                                                                                                  \
    inline const Class* q_func() const                                                                                 \
    {                                                                                                                  \
        return (static_cast< const Class* >(q_ptr));                                                                   \
    }                                                                                                                  \
    friend class Class;                                                                                                \
    Class* q_ptr;
#endif

/**
 * @def DA_DECLARE_PRIVATE
 * @brief 模仿Q_DECLARE_PRIVATE，但不用前置声明而是作为一个内部类
 *
 * 例如:
 *
 * @code
 * //header
 * class A
 * {
 *  DA_DECLARE_PRIVATE(A)
 * };
 * @endcode
 *
 * 其展开效果为：
 *
 * @code
 * class A{
 *  class PrivateData;
 *  friend class A::PrivateData;
 *  std::unique_ptr< PrivateData > d_ptr;
 * }
 * @endcode
 *
 * 这样前置声明了一个内部类PrivateData，在cpp文件中建立这个内部类的实现
 *
 * @code
 * //cpp
 * class A::PrivateData{
 *  DA_DECLARE_PUBLIC(A)
 *  PrivateData(A* p):q_ptr(p){
 *  }
 * };
 *
 * A::A():d_ptr(new PrivateData(this)){
 * }
 * @endcode
 *
 */
#ifndef DA_DECLARE_PRIVATE
#define DA_DECLARE_PRIVATE(classname)                                                                                  \
    class PrivateData;                                                                                                 \
    friend class classname::PrivateData;                                                                               \
    std::unique_ptr< PrivateData > d_ptr;                                                                              \
    inline PrivateData* d_func()                                                                                       \
    {                                                                                                                  \
        return (d_ptr.get());                                                                                          \
    }                                                                                                                  \
    inline const PrivateData* d_func() const                                                                           \
    {                                                                                                                  \
        return (d_ptr.get());                                                                                          \
    }
#endif

/**
 * @def DA_DECLARE_PUBLIC
 * @brief 模仿Q_DECLARE_PUBLIC
 *
 * 配套DA_DECLARE_PRIVATE使用
 */
#ifndef DA_DECLARE_PUBLIC
#define DA_DECLARE_PUBLIC(classname)                                                                                   \
    friend class classname;                                                                                            \
    classname* q_ptr { nullptr };                                                                                      \
    inline classname* q_func()                                                                                         \
    {                                                                                                                  \
        return (static_cast< classname* >(q_ptr));                                                                     \
    }                                                                                                                  \
    inline const classname* q_func() const                                                                             \
    {                                                                                                                  \
        return (static_cast< const classname* >(q_ptr));                                                               \
    }
#endif

/**
 * @def  DA_PIMPL_CONSTRUCT
 *
 * 配套DA_DECLARE_PRIVATE使用,在构造函数中构建privatedata
 */
#ifndef DA_PIMPL_CONSTRUCT
#define DA_PIMPL_CONSTRUCT d_ptr(std::make_unique< PrivateData >(this))
#endif

/**
 *@def DA_D
 *@brief impl获取指针，参考Q_D
 */
#ifndef DA_D
#define DA_D(pointerName) PrivateData* pointerName = d_func()
#endif

/**
 *@def DA_DC
 *@brief impl获取指针，参考Q_D
 */
#ifndef DA_DC
#define DA_DC(pointerName) const PrivateData* pointerName = d_func()
#endif

/**
 *@def DA_Q
 *@brief impl获取指针，参考Q_Q
 */
#ifndef DA_Q
#define DA_Q(classname, pointerName) classname* pointerName = q_func()
#endif

/**
 *@def DA_QC
 *@brief impl获取指针，参考Q_Q
 */
#ifndef DA_QC
#define DA_QC(classname, pointerName) const classname* pointerName = q_func()
#endif

/**
 *@def Qt5Qt6Compat_QXXEvent_Pos
 *@brief QXXEvent_Pos函数的兼容宏
 *
 *'pos' is deprecated: Use position().toPoint()
 *
 *qevent.h:740:5: note: 'pos' has been explicitly marked deprecated here
 *
 *qglobal.h:369:44: note: expanded from macro 'QT_DEPRECATED_VERSION_X_6_0'
 *
 *qglobal.h:281:33: note: expanded from macro 'QT_DEPRECATED_X'
 *
 */
#ifndef Qt5Qt6Compat_QXXEvent_Pos
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define Qt5Qt6Compat_QXXEvent_Pos(valuePtr) valuePtr->pos()
#else
#define Qt5Qt6Compat_QXXEvent_Pos(valuePtr) valuePtr->position().toPoint()
#endif
#endif

/**
 * @def Qt5Qt6Compat_fontMetrics_width
 * @brief QFontMetrics的字体宽度适配
 */
#ifndef Qt5Qt6Compat_fontMetrics_width
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
#define Qt5Qt6Compat_fontMetrics_width(fontMeter, str) fontMeter.width(str)
#else
#define Qt5Qt6Compat_fontMetrics_width(fontMeter, str) fontMeter.horizontalAdvance(str)
#endif
#endif
#endif  // GLOBALS_H
