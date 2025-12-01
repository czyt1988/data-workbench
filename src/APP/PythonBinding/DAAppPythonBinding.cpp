#include "DAAppPythonBinding.h"
#include "DAAppCore.h"
#include "DAPybind11InQt.h"

PYBIND11_EMBEDDED_MODULE(da_app, m)
{
    /* 5. 全局入口函数 */
    m.def("get_core", &DA::getAppCorePtr, "Return the application core interface (singleton)");
}
