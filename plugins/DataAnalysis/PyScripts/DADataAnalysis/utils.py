# -*- coding: utf-8 -*-

import os
from typing import List,Tuple,Optional
import pandas as pd
import numpy as np
# 这是DA自动内嵌的模块
# 获取datamanager
# datamanager = da_app.getCore().getDataManagerInterface()
# signal_handler，用于线程中操作界面，会发射操作到qt的队列中执行，如果在python线程中操作界面相关，需要通过此类实现
# signal_handler = da_app.getCore().getPythonSignalHandler()
# signal_handler.callInMainThread(add_data_in_main_thread)
import da_app,da_interface,da_data

def get_select_dataframe_and_subset_index(data: da_data.DAData = None) -> Optional[Tuple[da_data.DAData, pd.DataFrame, List[str]]]:
    """
    Get the selected dataframe and column indices
    获取选中的dataframe和选中的列索引
    """
    ui = da_app.getCore().getUiInterface()
    dataManager = da_app.getCore().getDataManagerInterface()
    
    if data is not None and isinstance(data, da_data.DAData):
        dadata = data
    else:
        dadata = dataManager.getOperateData()
    
    if not dadata:
        ui.addWarningLogMessage(_("Please select a dataset first"))  # cn: 请先选择一个数据集
        return None
    
    if not dadata.isDataFrame():
        ui.addWarningLogMessage(_("The selected data must be a table (DataFrame) format"))  # cn: 选中的数据必须是表格（DataFrame）格式
        return None
    
    df = dadata.toDataFrame()
    subset = None
    col_selected_index = dataManager.getOperateDataSeries()
    
    if col_selected_index and len(col_selected_index) > 0:
        subset = list(df.columns[col_selected_index])
    
    return dadata, df, subset