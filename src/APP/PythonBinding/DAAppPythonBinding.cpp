#include "DAAppPythonBinding.h"
#include "DAAppCore.h"
#include "DAPybind11InQt.h"

PYBIND11_EMBEDDED_MODULE(da_app, m)
{
    /* 5. 全局入口函数 */
    m.def("getCore",
          &DA::getAppCorePtr,
          "Return the application core interface (singleton)",
          pybind11::return_value_policy::reference);  // 务必要制定pybind11::return_value_policy::reference，否则pybind11会析构它
}
