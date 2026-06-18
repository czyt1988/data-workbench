# -*- coding: utf-8 -*-
"""将数据发布到 DataManager 面板的系统节点"""

import threading
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Parameter


@NodeDef(
    name="Output to DataManager",
    category="System / Data",
    icon="",
)
class DataToManagerNode:
    """将输入数据（推荐 DataFrame）发布到 DAWorkbench 的 DataManager 面板。"""

    data_name = Parameter(
        str,
        default="workflow_output",
        description="数据在 DataManager 面板中的显示名称",
    )

    class Inputs:
        data = Input("any", required=True, description="要发布到 DataManager 的数据")

    # 无输出：成功状态通过框架自动渲染在节点上

    def execute(self, inputs=None, params=None):
        if inputs is None:
            return False

        df = inputs.get("data")
        if df is None:
            return False

        if params is None:
            params = {}
        data_name = params.get("data_name", "workflow_output") or "workflow_output"

        try:
            import da_app
            import da_data
        except ImportError:
            return False

        try:
            core = da_app.getCore()
            data_mgr = core.getDataManagerInterface()

            def _publish():
                # 精确查找同名数据：命中则原地替换底层 DataFrame，
                # DAData 内部用 shared_ptr，所有持有同一 DAData 的地方共享同一底层对象，
                # 已打开的表格视图会通过 dataChanged(ChangeValue) 信号自动 refreshTable()。
                existing = data_mgr.findData(data_name, case_sensitive=True)
                if not existing.isNull():
                    existing.setPyObject(df)
                    existing.getDataManager().notifyDataChangedSignal(
                        existing, da_data.DataChangeType.Value
                    )
                else:
                    data = da_data.DAData(df)
                    data.setName(data_name)
                    data_mgr.addData_(data)

            if threading.current_thread() is threading.main_thread():
                _publish()
            else:
                signal_handler = core.getPythonSignalHandler()
                signal_handler.callInMainThread(_publish)

            return True
        except Exception:
            return False
