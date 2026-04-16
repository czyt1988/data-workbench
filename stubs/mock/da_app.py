"""
da_app 模块的 Mock 实现

此模块提供 Qt 应用程序入口点的 Mock 实现，包括核心接口获取和日志记录功能。
此实现工作在标准 Python 环境中（无需 Qt 或 C++ 程序）。
"""

from typing import Optional

# 尝试导入 da_interface 模块
try:
    from .da_interface import DACoreInterface
except ImportError:
    # 如果相对导入失败，尝试绝对导入
    try:
        from da_interface import DACoreInterface
    except ImportError:
        # 最后尝试从 stubs.mock 导入
        from stubs.mock.da_interface import DACoreInterface


# 全局变量用于缓存核心接口实例（单例模式）
_core_instance: Optional[DACoreInterface] = None


def getCore() -> DACoreInterface:
    """
    获取应用程序核心接口（单例模式）
    
    此函数返回应用程序核心接口的实例。采用单例模式，首次调用时创建实例，
    后续调用返回缓存的实例。
    
    Returns:
        DACoreInterface: 应用程序核心接口实例
    """
    global _core_instance
    
    if _core_instance is None:
        print("[Mock da_app] 创建核心接口实例（单例）")
        _core_instance = DACoreInterface()
    else:
        print("[Mock da_app] 返回已缓存的核心接口实例")
    
    return _core_instance


def addInfoLogMessage(msg: str) -> None:
    """
    添加信息日志消息
    
    此函数将信息级别的日志消息添加到应用程序日志系统。
    在 Mock 实现中，直接打印到控制台。
    
    Args:
        msg: 日志消息文本
    """
    print(f"[INFO] {msg}")


def addWarningLogMessage(msg: str) -> None:
    """
    添加警告日志消息
    
    此函数将警告级别的日志消息添加到应用程序日志系统。
    在 Mock 实现中，直接打印到控制台。
    
    Args:
        msg: 日志消息文本
    """
    print(f"[WARNING] {msg}")


def addCriticalLogMessage(msg: str) -> None:
    """
    添加严重日志消息
    
    此函数将严重级别的日志消息添加到应用程序日志系统。
    在 Mock 实现中，直接打印到控制台。
    
    Args:
        msg: 日志消息文本
    """
    print(f"[CRITICAL] {msg}")