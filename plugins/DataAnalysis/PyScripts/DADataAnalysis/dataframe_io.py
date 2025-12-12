# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
import pandas as pd
from pathlib import Path
import numpy as np
import traceback
import threading
from DAWorkbench.da_logger import log_function_call  # type: ignore # 引入装饰器
import DAWorkbench.thread_status_manager as tsm
import DAWorkbench.utils as daUtils
import chardet
# 这是DA自动内嵌的模块
# 获取datamanager
# datamanager = da_app.getCore().getDataManagerInterface()
# signal_handler，用于线程中操作界面，会发射操作到qt的队列中执行，如果在python线程中操作界面相关，需要通过此类实现
# signal_handler = da_app.getCore().getPythonSignalHandler()
# signal_handler.callInMainThread(add_data_in_main_thread)
import da_app,da_interface,da_data

'''
本文件da_打头的变量和函数属于da系统的默认函数，如果改动会导致da系统异常
'''

def detect_encoding(file_path, chunk_size=1024):
    """
    检测文件的编码，适用于大文件和小文件。

    参数:
        file_path (str): 文件路径。
        chunk_size (int): 每次读取的字节数，默认 1024 字节。

    返回:
        str: 检测到的文件编码。如果检测失败，返回默认编码 'utf-8'。
    """
    detector = chardet.UniversalDetector()  # 创建编码检测器

    with open(file_path, 'rb') as f:
        file_size = f.seek(0, 2)  # 获取文件大小
        f.seek(0)  # 回到文件开头

        if file_size <= chunk_size:
            # 如果是小文件，直接读取整个文件
            chunk = f.read()
            detector.feed(chunk)
        else:
            # 如果是大文件，分块读取
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    break
                detector.feed(chunk)
                if detector.done:
                    break

    detector.close()  # 关闭检测器

    # 获取检测结果
    result = detector.result
    encoding = result['encoding']
    confidence = result['confidence']  # 检测结果的置信度

    # 如果置信度过低或编码为 None，使用默认编码 'utf-8'
    if encoding is None or confidence < 0.5:
        return 'utf-8'

    return encoding


def export_datamanager_thread(file_path: str, type: str = 'csv',export_all:bool = True)-> str:
    """
    将dataManager导出为type指定的文件。

    参数:
        file_path (str): 导出文件的文件夹。
        type (str): 导出的文件类型，可选 'csv' 或 'excel'。
    Returns:
        str: 任务id，可以通过这个id，获取这个任务的进度信息，None表示启动失败
    """
    if not os.path.exists(file_path):
        os.makedirs(file_path, exist_ok=True)
    if not os.path.isdir(file_path):
        raise Exception(f"The specified path is not a valid folder: {file_path}")
    datamanager = da_app.getCore().getDataManagerInterface()
    if not datamanager:
        raise Exception("DataManagerInterface is not available")
    # 获取数据
    dataframes_dict = {}
    if export_all:
        dataframes_dict = datamanager.getAllDataframes()
    else:
        dataframes_dict = datamanager.getSelectDataframes()
    # 创建一个状态对象
    taskid,status = tsm.create_task_with_status("export datamanager files")
    file_type = type.strip().lower()
    def save_dataframes_worker():
        """线程工作函数：实际执行数据的保存操作"""
        status.start()
        error_msg = None
        try:
            total_count = len(dataframes_dict)
            for index, (name, df) in enumerate(dataframes_dict.items()):
                status.update_progress(index / total_count * 100,f"export: {name}.{file_type}")
                save_file_path = os.path.join(file_path, f"{name}.{file_type}")
                # 按类型导出
                if file_type == 'csv':
                    df.to_csv(save_file_path, index=False)  # 去掉索引列
                elif file_type == 'excel':
                    df.to_excel(save_file_path, index=False, engine='openpyxl')
                elif file_type == 'parquet':
                    df.to_parquet(save_file_path, index=False)
                elif file_type == 'feather':
                    df.to_feather(save_file_path)
                elif file_type == 'pickle':
                    df.to_pickle(save_file_path)
                elif file_type == 'html':
                    df.to_html(save_file_path, index=False)
                elif file_type == 'json':
                    df.to_json(save_file_path, force_ascii=False)
            # 写入用户数据
            status.update_custom_data("file_count",total_count)
            status.finish(True,f"export success {total_count} files")
        except Exception as e:
            error_msg = f"error: {str(e)}\n{traceback.format_exc()}"
            status.finish(False,error_msg)
    try:
        # 创建并启动线程
        save_thread = threading.Thread(target=save_dataframes_worker, daemon=True)
        save_thread.start()
        return taskid
    except Exception as e:
        status.finish(False,"unknown error")
        return None