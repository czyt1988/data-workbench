# -*- coding: utf-8 -*-
import os
import sys
import da_app,da_interface

def run_in_main_thread(func):
    """
    装饰器：确保被装饰的函数在Qt主线程中执行
    
    用法：
    @run_in_main_thread
    def update_ui():
        # 这里可以安全操作Qt UI
        pass
    """
    def wrapper(*args, **kwargs):
        signal_handler = da_app.getCore().getPythonSignalHandler()
        if signal_handler:
            signal_handler.callInMainThread(lambda: func(*args, **kwargs))
        else:
            func(*args, **kwargs)  # 降级处理
    
    return wrapper


def call_in_main_thread(func, *args, **kwargs):
    """
    立即调度函数到主线程执行
    
    参数：
    func: 要执行的函数
    *args, **kwargs: 函数的参数
    
    返回：
    无返回值（如果需要返回值，需要更复杂的实现）
    """
    signal_handler = da_app.getCore().getPythonSignalHandler()
    if signal_handler:
        signal_handler.callInMainThread(lambda: func(*args, **kwargs))
    else:
        func(*args, **kwargs)