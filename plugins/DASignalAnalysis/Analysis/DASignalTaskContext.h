#ifndef DASIGNALTASKCONTEXT_H
#define DASIGNALTASKCONTEXT_H
#include <QtGlobal>
#include "Signal/DASignal.h"

namespace DA
{
/**
 * @brief 后台任务上下文（信号 + 通道 + 参数版本）
 *
 * 评审 U3/U4（v2 方案 B4）要求任何异步结果都不能写到别的信号或通道上。
 * 计算任务发起时把身份随任务一起保存在 worker 的任务表里，结果回传时原样带回，
 * 界面层据此校验「信号还在、通道仍匹配、参数版本未过期」后再回填。
 */
struct DASignalTaskContext
{
    DASignalPtr signal;              ///< 目标信号（持有 shared_ptr，任务期间信号不会被销毁）
    int channel { 0 };               ///< 目标通道
    quint64 paramsVersion { 0 };     ///< 发起计算时的参数版本号（参数变更后旧结果被丢弃）
};
}  // namespace DA

#endif  // DASIGNALTASKCONTEXT_H
