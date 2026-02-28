#ifndef DAUNIQUEIDGENERATER_H
#define DAUNIQUEIDGENERATER_H
#include "DAUtilsAPI.h"
#include <cstdint>
namespace DA
{

/**
 * @brief id生成器
 */
class DAUTILS_API DAUniqueIDGenerater
{
public:
	DAUniqueIDGenerater();
	/**
	 * @brief 生成当前进程唯一uint64_t类型的id
	 * @return
	 */
	static uint64_t id_uint64();
};

}
#endif  // DAUNIQUEIDGENERATER_H
