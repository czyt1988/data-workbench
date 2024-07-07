#ifndef DAPYBIND11INQT_H
#define DAPYBIND11INQT_H
#include "DAPyBindQtGlobal.h"

/**
 * @brief 通过此头文件引入python相关的头文件，避免slots宏定义冲突
 *
 * 此头文件屏蔽了slots宏后引入了python相关的头文件，再启动slots宏
 */

#undef slots
#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#endif
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "pybind11/cast.h"
#include "pybind11/embed.h"
#include "pybind11/stl.h"
#include "pybind11/chrono.h"
#define slots Q_SLOTS

#endif  // DAPYBIND11INQT_H
