#include "DAPathUtil.h"
#include <iostream>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <filesystem>
#else
#include <unistd.h>
#include <limits.h>
#include <filesystem>
#endif
namespace DA
{
DAPathUtil::DAPathUtil()
{
}

QString DAPathUtil::getExecutablePath()
{
	std::string executablePath = get_executable_path();
	// 这时文本是系统编码的，要转换为utf-8
	return QString::fromLocal8Bit(executablePath.c_str());
}

std::string DAPathUtil::get_executable_path()
{
#if defined(_WIN32) || defined(_WIN64)
	char buffer[ MAX_PATH ];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string fullPath(buffer);
	std::filesystem::path path(fullPath);
	return path.parent_path().string();
#else
	char buffer[ PATH_MAX ];
	ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
	if (count == -1) {
		return "";  // Error occurred
	}
	std::string fullPath(buffer, count);
	std::filesystem::path path(fullPath);
	return path.parent_path().string();
#endif
}
}
