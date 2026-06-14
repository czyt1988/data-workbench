# -*- coding: utf-8 -*-
"""
输出到数据管理区节点 — 将 DataFrame 发布到 DAWorkbench 的 data_manager 面板

本节点接收上游 DataFrame，通过 da_app/da_data Python 接口将其注册到应用的数据管理区，
实现工作流节点与 DAWorkbench UI 的交互。

用户可设置 data_name 参数，该名称即为数据在 data_manager 面板中的显示名称。
当工作流在后台线程执行时，通过 callInMainThread 保证 UI 操作的线程安全。
"""

import threading
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Output to DataManager", category="数据操作", icon="data_to_manager")
class DataToManagerNode:
    """将 DataFrame 发布到应用的数据管理区"""

    data_name = Parameter(str, default="workflow_output", description="数据名称，显示在数据管理区")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入 DataFrame")

    class Outputs:
        success = Output("bool", description="是否成功发布到数据管理区")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        """
        执行数据发布

        将输入的 DataFrame 通过 da_app/da_data 接口添加到应用的数据管理区。
        使用 addData_() 以支持 undo/redo。当工作流在后台线程执行时，
        通过 callInMainThread 确保 UI 操作在主线程完成。

        :param inputs: 输入数据字典，需包含 "data" 键对应的 DataFrame
        :param params: 参数字典，需包含 "data_name" 键对应的数据名称
        :return: 执行成功返回 True，失败返回 False
        """
        if inputs is None:
            self._output_data["success"] = False
            return False

        df = inputs.get("data")
        if df is None:
            self._output_data["success"] = False
            return False

        if params is None:
            params = {}
        data_name = params.get("data_name", "workflow_output") or "workflow_output"

        try:
            import da_app
            import da_data
        except ImportError:
            self._output_data["success"] = False
            return False

        try:
            core = da_app.getCore()
            data_mgr = core.getDataManagerInterface()

            # 创建 DAData 并设置名称
            data = da_data.DAData(df)
            data.setName(data_name)

            # 判断是否在主线程，决定是否需要线程安全调度
            if threading.current_thread() is threading.main_thread():
                data_mgr.addData_(data)
            else:
                # 后台线程：通过 callInMainThread 保证 UI 操作在主线程执行
                signal_handler = core.getPythonSignalHandler()

                def _add_in_main_thread():
                    data_mgr.addData_(data)

                signal_handler.callInMainThread(_add_in_main_thread)

            self._output_data["success"] = True
            return True
        except Exception:
            self._output_data["success"] = False
            return False
