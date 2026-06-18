#ifndef DAAPPPROJECTACTIONPOLICY_H
#define DAAPPPROJECTACTIONPOLICY_H

namespace DA
{
/**
 * @brief 工程保存提示框的用户选择
 */
enum class DAAppSavePromptChoice
{
    Save,     ///< 保存后继续
    Discard,  ///< 放弃修改并继续
    Cancel    ///< 取消当前操作
};

/**
 * @brief 关闭窗口前的决策
 */
enum class DAAppCloseAction
{
    CloseDirectly,   ///< 直接关闭
    SaveThenClose,   ///< 先保存，再关闭
    CancelClosing    ///< 取消关闭
};

/**
 * @brief 打开工程前的准备动作
 */
enum class DAAppOpenPreparation
{
    OpenDirectly,             ///< 直接打开
    SaveThenOpen,             ///< 先保存当前工程，再打开
    ConfirmReplaceThenOpen,   ///< 先确认是否替换当前工程，再打开
    CancelOpening             ///< 取消打开
};

/**
 * @brief 根据保存提示框选择，判断关闭窗口前的动作
 * @param isDirty 当前工程是否有未保存修改
 * @param choice 用户选择
 * @return 关闭动作
 */
inline DAAppCloseAction resolveAppCloseAction(bool isDirty, DAAppSavePromptChoice choice)
{
    if (!isDirty) {
        return DAAppCloseAction::CloseDirectly;
    }
    switch (choice) {
    case DAAppSavePromptChoice::Save:
        return DAAppCloseAction::SaveThenClose;
    case DAAppSavePromptChoice::Discard:
        return DAAppCloseAction::CloseDirectly;
    case DAAppSavePromptChoice::Cancel:
    default:
        return DAAppCloseAction::CancelClosing;
    }
}

/**
 * @brief 根据当前工程状态，判断打开新工程前的准备动作
 * @param hasProjectContent 当前是否存在已加载工程
 * @param isDirty 当前工程是否有未保存修改
 * @param choice 用户对未保存修改的选择
 * @return 打开前准备动作
 */
inline DAAppOpenPreparation resolveAppOpenPreparation(bool hasProjectContent,
                                                      bool isDirty,
                                                      DAAppSavePromptChoice choice)
{
    if (isDirty) {
        switch (choice) {
        case DAAppSavePromptChoice::Save:
            return DAAppOpenPreparation::SaveThenOpen;
        case DAAppSavePromptChoice::Discard:
            return DAAppOpenPreparation::OpenDirectly;
        case DAAppSavePromptChoice::Cancel:
        default:
            return DAAppOpenPreparation::CancelOpening;
        }
    }
    if (hasProjectContent) {
        return DAAppOpenPreparation::ConfirmReplaceThenOpen;
    }
    return DAAppOpenPreparation::OpenDirectly;
}

}  // namespace DA

#endif  // DAAPPPROJECTACTIONPOLICY_H
