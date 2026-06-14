# -*- coding: utf-8 -*-
"""
工作流调试开关模块

集中管理工作流调试标志位，所有工作流模块（executor、signal_manager、node_def）
统一从此模块导入调试标志和输出函数。

启用调试：将 _WF_DEBUG 设为 True
关闭调试：将 _WF_DEBUG 设为 False

对应 C++ 侧宏 DA_WORKFLOW_DEBUG (src/DAPyWorkFlow/DAPyWorkFlowAPI.h)
"""

# ============================================================
# 工作流调试开关
# 设为 True 可在控制台输出工作流执行的详细日志，调试完成后改回 False
# ============================================================
WF_DEBUG = True


def wf_dbg(tag: str, *args):
    """
    工作流调试输出

    :param tag: 模块标签（如 "Exec"、"Signal"、"Node"）
    :param args: 输出内容
    """
    if WF_DEBUG:
        print(f"[WF-DBG][{tag}]", *args)
