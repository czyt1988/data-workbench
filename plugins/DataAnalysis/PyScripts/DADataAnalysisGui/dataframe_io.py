# -*- coding: utf-8 -*-
"""
数据 I/O 模块（GUI 层）

GUI 相关的导入/导出函数（线程化、数据管理器集成）。
纯函数已迁移至 DADataAnalysisCore.io，此处通过重导出保持向后兼容。
"""

import os
from typing import List, Dict, Optional
import pandas as pd
import traceback
import threading

from DAWorkbench.DAPyBase.da_logger import log_function_call  # type: ignore
import DAWorkbench.DAPyBase.thread_status_manager as tsm
import da_app, da_interface, da_data

# 从 Core 重导出纯函数，保持 C++ Worker 调用的向后兼容性
from DADataAnalysisCore.io import detect_encoding, support_file_filters_list, support_file_filters, export_data


def export_datamanager_thread(file_path: str, type: str = 'csv', export_all: bool = True) -> str:
    """
    将 dataManager 导出为 type 指定的文件（在独立线程中执行）。

    :param file_path: 导出文件的文件夹路径
    :param type: 导出文件类型（csv/excel/parquet 等）
    :param export_all: True 导出全部，False 仅导出选中
    :return: 任务 id，None 表示启动失败
    """
    if not os.path.exists(file_path):
        os.makedirs(file_path, exist_ok=True)
    if not os.path.isdir(file_path):
        errstr = _("The specified path is not a valid folder: {file_path}")
        errstr.format(file_path=file_path)
        raise Exception(errstr)
    datamanager = da_app.getCore().getDataManagerInterface()
    if not datamanager:
        raise Exception("DataManagerInterface is not available")
    dataframes_dict = {}
    if export_all:
        dataframes_dict = datamanager.getAllDataframes()
    else:
        dataframes_dict = datamanager.getSelectDataframes()
    taskid, status = tsm.create_task_with_status(_("export datamanager files"))
    file_type = type.strip().lower()

    def save_dataframes_worker():
        status.start()
        try:
            total_count = len(dataframes_dict)
            for index, (name, df) in enumerate(dataframes_dict.items()):
                status.update_progress(index / total_count * 100, _("export: {name}.{file_type}").format(name=name, file_type=file_type))
                save_file_path = os.path.join(file_path, f"{name}.{file_type}")
                export_data(df, save_file_path, file_type)
            os.startfile(file_path)
            info_str = _("export success {total_count} files")
            info_str = info_str.format(total_count=total_count)
            status.finish(True, info_str)
        except Exception as e:
            status.finish(False, _("Failed To Export dataframe: {err1}\n{err2}").format(err1=str(e), err2=traceback.format_exc()))

    try:
        save_thread = threading.Thread(target=save_dataframes_worker, daemon=True)
        save_thread.start()
        return taskid
    except Exception as e:
        status.finish(False, _("unknown error"))
        return None


def export_datamanager_to_excel_thread(file_path: str, export_all: bool = True) -> str:
    """
    在独立线程中将数据区的内容保存到 Excel 文件（每个 DataFrame 对应一个 sheet）。

    :param file_path: 输出 Excel 文件路径
    :param export_all: 是否全部导出
    :return: 任务 id，None 表示启动失败
    """
    datamanager = da_app.getCore().getDataManagerInterface()
    if not datamanager:
        raise Exception("DataManagerInterface is not available")
    dataframes_dict = {}
    if export_all:
        dataframes_dict = datamanager.getAllDataframes()
    else:
        dataframes_dict = datamanager.getSelectDataframes()

    taskid, status = tsm.create_task_with_status("export to one excel thread")

    def save_excel_worker():
        status.start()
        try:
            with pd.ExcelWriter(file_path, engine='openpyxl', mode='w') as writer:
                total_count = len(dataframes_dict)
                for index, (sheet_name, df) in enumerate(dataframes_dict.items()):
                    str_info = _("writing sheet: {sheet_name}")
                    str_info = str_info.format(sheet_name=sheet_name)
                    status.update_progress(index / total_count * 100, str_info)
                    clean_sheet_name = sheet_name.replace('/', '_').replace('\\', '_').replace('*', '_').replace('?', '_').replace('[', '_').replace(']', '_')
                    df.to_excel(writer, sheet_name=clean_sheet_name, index=False)

            if not os.path.exists(file_path):
                raise Exception(_("Failed To Export Excel File"))
            status.finish(True, _("export success {total_count} sheets, file: {file_path}").format(total_count=total_count, file_path=file_path))
        except Exception as e:
            status.finish(False, _("Failed To Export Excel File: {err1}\n{err2}").format(err1=str(e), err2=traceback.format_exc()))
            if os.path.exists(file_path):
                try:
                    os.remove(file_path)
                except:
                    pass

    try:
        save_thread = threading.Thread(target=save_excel_worker, daemon=True)
        save_thread.start()
        return taskid
    except Exception as e:
        status.finish(False, _("unknown error"))
        return None
