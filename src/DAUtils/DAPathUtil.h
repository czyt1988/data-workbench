#ifndef DAPATHUTIL_H
#define DAPATHUTIL_H
#include "DAUtilsAPI.h"
#include <QString>
#include <string>

namespace DA
{

class DAUTILS_API DAPathUtil
{
public:
	DAPathUtil();
	// 获取程序运行路径
	static QString getExecutablePath();
	//
	static std::string get_executable_path();
};
}

#endif  // DAPATHUTIL_H
