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
	 * @brief 生成当前进程唯一id
	 * @return
	 */
	static uint64_t id();
};

}
#endif  // DAUNIQUEIDGENERATER_H
