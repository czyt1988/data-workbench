#ifndef DADATAGLOBAL_H
#define DADATAGLOBAL_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DADATA_BUILDLIB)
#define DADATA_API Q_DECL_EXPORT
#else
#define DADATA_API Q_DECL_IMPORT
#endif

namespace DA
{
/**
 * @brief DAData 模块进行meta类型初始化
 *
 * 此函数的实现在DAData.cpp
 */
void DADATA_API da_data_register_metatypes();
}
#endif  // DAPYDATAANALYSISGLOBAL_H
