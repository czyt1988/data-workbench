#ifndef DACONFIGS_H
#define DACONFIGS_H
/**
 * @file 此文档由cmake根据CMakeLists.txt自动生成，任何在此文件上的改动都将覆写
 */


/**
 * @def 版本号-major（数字）
 */
#ifndef DA_VERSION_MAJOR
#define DA_VERSION_MAJOR 0
#endif

/**
 * @def 版本号-minor（数字）
 */
#ifndef DA_VERSION_MINOR
#define DA_VERSION_MINOR 0
#endif

/**
 * @def 版本号-patch（数字）
 */
#ifndef DA_VERSION_PATCH
#define DA_VERSION_PATCH 2
#endif

/**
 * @def 版本号（字符串）
 */
#ifndef DA_VERSION
#define DA_VERSION "0.0.2"
#endif

/**
 * @def 编译的日期（字符串）
 */
#ifndef DA_COMPILE_DATETIME
#define DA_COMPILE_DATETIME "240520"
#endif

/**
 * @def 编译的年份（字符串）
 */
#ifndef DA_COMPILE_DATETIME_YEAR
#define DA_COMPILE_DATETIME_YEAR "24"
#endif

/**
 * @def 编译的月份（字符串）
 */
#ifndef DA_COMPILE_DATETIME_MONTH
#define DA_COMPILE_DATETIME_MONTH "05"
#endif

/**
 * @def 编译的日期（字符串）
 */
#ifndef DA_COMPILE_DATETIME_DAY
#define DA_COMPILE_DATETIME_DAY "20"
#endif

/**
 * @def 是否开启python（bool）
 */
#ifndef DA_ENABLE_PYTHON
#define DA_ENABLE_PYTHON 1
#endif

/**
 * @def 工程构建的名称（字符串）
 */
#ifndef DA_PROJECT_NAME
#define DA_PROJECT_NAME "DAWorkbench"
#endif

#endif  // DACONFIGS_H
