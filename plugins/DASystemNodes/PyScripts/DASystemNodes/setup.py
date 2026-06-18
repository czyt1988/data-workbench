# -*- coding: utf-8 -*-
"""
DASystemNodes 包安装配置

用于支持 pip 安装和 entry_points 发现。
"""

from setuptools import setup, find_packages

setup(
    name="DASystemNodes",
    version="1.0.0",
    packages=find_packages(),
    entry_points={
        "data_workbench.plugin": [
            "DASystemNodes = DASystemNodes",
        ],
    },
)
