"""
da_app 模块类型定义

此模块是应用程序的入口点模块，提供核心接口获取和日志功能。
用户通过调用 da_app.getCore() 获取核心接口，然后导航到其他模块。
"""

from da_interface import DACoreInterface


def getCore() -> DACoreInterface:
    """
    获取应用程序核心接口（单例）

    返回应用程序的核心接口实例，通过此接口可以访问其他所有模块接口。
    此函数使用引用返回策略，pybind11 不会析构返回的对象。

    Returns:
        DACoreInterface: 应用程序核心接口实例
    """
    ...


def addInfoLogMessage(msg: str) -> None:
    """
    添加信息日志消息

    在应用程序的日志窗口中显示信息级别的消息。

    Args:
        msg: 要显示的信息消息
    """
    ...


def addWarningLogMessage(msg: str) -> None:
    """
    添加警告日志消息

    在应用程序的日志窗口中显示警告级别的消息。

    Args:
        msg: 要显示的警告消息
    """
    ...


def addCriticalLogMessage(msg: str) -> None:
    """
    添加严重日志消息

    在应用程序的日志窗口中显示严重级别的消息。

    Args:
        msg: 要显示的严重消息
    """
    ...