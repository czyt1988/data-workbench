#ifndef DAAPPCONFIGAPPLIER_H
#define DAAPPCONFIGAPPLIER_H

#include "SettingPages/DAAppConfig.h"

namespace DA
{

class DAAppCore;

/**
 * @brief 把配置的某一分节施加到运行时子系统
 *
 * 只读配置、只改运行时状态，禁止反向写配置（否则会与 DAAppConfig 的
 * sectionChanged 信号形成回环）。
 * @param cfg 配置对象
 * @param core 应用核心
 * @param section 要应用的分节
 */
void applyAppSettings(const DAAppConfig& cfg, DAAppCore* core, DAAppConfig::Section section);

/**
 * @brief 把全部配置施加到运行时子系统（启动阶段使用）
 * @param cfg 配置对象
 * @param core 应用核心
 */
void applyAllAppSettings(const DAAppConfig& cfg, DAAppCore* core);

}  // namespace DA

#endif  // DAAPPCONFIGAPPLIER_H
