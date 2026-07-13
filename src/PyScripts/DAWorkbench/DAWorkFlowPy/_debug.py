# -*- coding: utf-8 -*-
"""
工作流调试开关模块

集中管理工作流调试标志位，所有工作流模块（executor、signal_manager、node_def）
统一从此模块导入调试标志和输出函数。

启用调试：设置环境变量 DA_WF_DEBUG=1
关闭调试：默认关闭（DA_WF_DEBUG 未设置或为 0/false/no）

对应 C++ 侧宏 DA_WORKFLOW_DEBUG (src/DAPyWorkFlow/DAPyWorkFlowAPI.h)
"""
import os

# ============================================================
# 工作流调试开关
# 通过环境变量 DA_WF_DEBUG=1 启用调试输出，默认关闭
# ============================================================
WF_DEBUG = os.environ.get("DA_WF_DEBUG", "").lower() in ("1", "true", "yes")


def wf_dbg(tag: str, *args):
    """
    工作流调试输出

    :param tag: 模块标签（如 "Exec"、"Signal"、"Node"）
    :param args: 输出内容
    """
    if WF_DEBUG:
        print(f"[WF-DBG][{tag}]", *args)
