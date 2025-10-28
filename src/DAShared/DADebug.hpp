#ifndef DADEBUG_H
#define DADEBUG_H
#include <QDebug>

#ifdef DA_FILE_DEBUG_ENABLED
#undef DA_FILE_DEBUG_ENABLED
#define DA_FILE_DEBUG_ENABLED 1  // 1=启用, 0=禁用
#endif

/**
  *@def 在每个需要打印的文件上如下设置即可控制这个文件的打印
  *
  *@code
  *#undef DA_FILE_DEBUG_ENABLED
  *#define DA_FILE_DEBUG_ENABLED 1  // 1=启用, 0=禁用
  *@endcode
  */
#if DA_FILE_DEBUG_ENABLED
    #define DADebug qDebug()
#else
    #define DADebug while(false) qDebug()
#endif

#endif  // DADEBUG_H
