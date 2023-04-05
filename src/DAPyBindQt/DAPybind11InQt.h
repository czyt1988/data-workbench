#ifndef DAPYBIND11INQT_H
#define DAPYBIND11INQT_H
#include "DAPyBindQtGlobal.h"
#undef slots
#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#endif
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "pybind11/cast.h"
#define slots Q_SLOTS

#endif  // DAPYBIND11INQT_H
