# -*- coding: utf-8 -*-
import os
import sys
from loguru import logger
import inspect

'''
本文件da_打头的变量和函数属于da系统的默认函数，如果改动会导致da系统异常

此文件封装日志相关的操作
'''
_initialized = False

def setup_logging():
    global _initialized
    if _initialized:
        return logger

    appdata = os.getenv('APPDATA') or os.path.expanduser("~")
    da_log_path = os.path.join(appdata, 'DAWorkBench', 'log')
    os.makedirs(da_log_path, exist_ok=True)
    log_file = os.path.join(da_log_path, "da_pyscript.log")

    logger.add(log_file, rotation="10 MB", level="DEBUG", enqueue=True)
    _initialized = True
    return logger

def shutdown_logging():
    global _initialized
    if not _initialized:
        return
    
    logger.remove()
    # ... 其他清理逻辑
    _initialized = False
    

def log_function_call(func):
    """
    装饰器：自动记录函数调用时的所有参数。
    """
    def wrapper(*args, **kwargs):
        # 获取函数签名
        sig = inspect.signature(func)
        # 绑定参数
        bound_args = sig.bind(*args, **kwargs)
        bound_args.apply_defaults()  # 应用默认值

        # 记录函数名和参数
        logger.debug(f"Calling {func.__name__} with arguments: {bound_args.arguments}")
        return func(*args, **kwargs)
    return wrapper

@log_function_call
def fun_test(a:str,b:int,c:bool):
    pass

if __name__ == '__main__':
    # __tst_insert_column()
    fun_test('123',12,True)
