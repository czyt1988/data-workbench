# -*- coding: utf-8 -*-
"""System node: publish data to the DataManager panel"""

import threading
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Parameter


@NodeDef(
    name="Output to DataManager",
    category=_("System / Data"),  # cn:系统 / 数据
    icon="",
    description=_("Publishes input data to the DAWorkbench DataManager panel. If data with the same name already exists, it is updated in place; otherwise a new entry is created. Recommended for DataFrame data."),  # cn:将输入数据发布到 DAWorkbench DataManager 面板。若同名数据已存在则原地更新，否则新建条目。推荐用于 DataFrame 数据。
)
class DataToManagerNode:
    """Publish input data (DataFrame recommended) to the DAWorkbench DataManager panel."""

    data_name = Parameter(
        str,
        default="workflow_output",
        description=_("Display name of the data in the DataManager panel"),  # cn:数据在 DataManager 面板中的显示名称
    )

    class Inputs:
        data = Input("any", required=True, description=_("Data to publish to DataManager"))  # cn:要发布到 DataManager 的数据

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
