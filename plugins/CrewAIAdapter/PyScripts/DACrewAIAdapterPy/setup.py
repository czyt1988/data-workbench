# -*- coding: utf-8 -*-
"""
DACrewAIAdapterPy 安装配置

通过 entry_points 注册到 data_workbench.plugin 组，
使 DANodeRegistry 可通过 importlib.metadata 自动发现本包中的节点。
"""

from setuptools import setup, find_packages

setup(
    name="DACrewAIAdapterPy",
    version="1.0.0",
    description="AI Agent CrewAI 适配器节点包 — 提供 Agent/Task/Crew/Tool 工作流节点",
    author="DA WorkBench Team",
    packages=find_packages(),
    python_requires=">=3.7",
    install_requires=[
        "DAWorkbench>=1.0.0",
    ],
    extras_require={
        "crewai": ["crewai>=0.1.0"],
    },
    entry_points={
        "data_workbench.plugin": [
            "DACrewAIAdapterPy = DACrewAIAdapterPy",
        ],
    },
)
