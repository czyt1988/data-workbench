# -*- coding: utf-8 -*-
"""
DADataAnalysisPy 安装配置

通过 entry_points 注册到 data_workbench.plugin 组，
使 DANodeRegistry 可通过 importlib.metadata 自动发现本包中的节点。
"""

from setuptools import setup, find_packages

setup(
    name="DADataAnalysisPy",
    version="1.0.0",
    description="数据分析示例节点包 — 基于 pandas 的数据处理工作流节点",
    author="DA WorkBench Team",
    packages=find_packages(),
    python_requires=">=3.7",
    install_requires=[
        "pandas",
    ],
    extras_require={
        "plot": ["matplotlib"],
        "export": ["openpyxl", "pyarrow"],  # Excel 和 Parquet/Feather 支持
    },
    entry_points={
        "data_workbench.plugin": [
            "DADataAnalysisPy = DADataAnalysisPy",
        ],
    },
)