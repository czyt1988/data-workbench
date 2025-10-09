#include "DAAppUtils.h"
#include "DAGuiAPI.h"
#include "DADataAPI.h"
#include "DAWorkFlowAPI.h"
namespace DA
{
/**
 * @brief 注册所有模块的元对象
 */
void app_register_all_metatypes()
{
    da_gui_register_metatypes();
    da_data_register_metatypes();
    da_workflow_register_metatypes();
}
}
