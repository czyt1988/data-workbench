#ifndef DAAPPPYTHONBINDING_H
#define DAAPPPYTHONBINDING_H
#include <string>
// 这里主要是要导出的函数实现
namespace DA
{
// 添加信息在程序的日志窗口里显示
void addInfoLogMessage(const std::string& msg);
// 添加信息在程序的日志窗口里显示
void addWarningLogMessage(const std::string& msg);
// 添加信息在程序的日志窗口里显示
void addCriticalLogMessage(const std::string& msg);
}
#endif  // DAAPPPYTHONBINDING_H
