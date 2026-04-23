#ifndef DAPYWORKFLOWENUMSTRINGUTILS_H
#define DAPYWORKFLOWENUMSTRINGUTILS_H
#include "DAEnumStringUtils.hpp"
#include "DAPyWorkFlowAPI.h"
#include "DAPyNodeState.h"
/**
 * @file Python工作流枚举字符串转换类
 *
 * 本文件提供了一组工具，用于在Python工作流相关枚举类型和字符串之间进行相互转换。
 * 通过定义的@sa enumToString 和@sa stringToEnum函数，可以轻松地将枚举值转换为字符串，
 * 或将字符串转换回枚举值。
 *
 * 示例用法：
 * @code
 * // 枚举转字符串
 * QString str = enumToString(DA::Idle);
 *
 * // 字符串转枚举
 * DA::DAPyNodeState val1 = stringToEnum(str, DA::Idle);  // 指定默认值
 * DA::DAPyNodeState val2 = stringToEnum<DA::DAPyNodeState>(str);  // 使用模板参数指定枚举类型
 * @endcode
 *
 * 通过这种方式，可以方便地在枚举和字符串之间进行转换，适用于配置文件解析、
 * 网络数据传输、用户输入处理等多种场景。
 */

// ------------------------------------------
// DA::DAPyNodeState
// ------------------------------------------
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::DAPyNodeState)

#endif  // DAPYWORKFLOWENUMSTRINGUTILS_H