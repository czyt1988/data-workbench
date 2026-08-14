#include "DAAgentPromptOps.h"

namespace DA
{
/// 析构函数定义锚定 vtable 于 DAUtils DLL，保证跨 DLL 指针调用安全
DAAgentPromptOps::~DAAgentPromptOps() = default;
} // namespace DA
