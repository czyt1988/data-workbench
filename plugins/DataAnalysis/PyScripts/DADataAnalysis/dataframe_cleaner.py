# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
import pandas as pd
from pathlib import Path
import numpy as np
import traceback
import threading
from loguru import logger
from DAWorkbench.da_logger import log_function_call  # type: ignore # 引入装饰器
import DAWorkbench.thread_status_manager as tsm
import DAWorkbench.property_config_builder as porpCfgBuilder
# 这是DA自动内嵌的模块
# 获取datamanager
# datamanager = da_app.getCore().getDataManagerInterface()
# signal_handler，用于线程中操作界面，会发射操作到qt的队列中执行，如果在python线程中操作界面相关，需要通过此类实现
# signal_handler = da_app.getCore().getPythonSignalHandler()
# signal_handler.callInMainThread(add_data_in_main_thread)
import da_app,da_interface,da_data


def dropna(dadata:da_data.DAData, col_selected_index:List[int]):
    """
    删除缺失值
    :return: 删除的行数
    """
    ui = da_app.getCore().getUiInterface()
    if not dadata:
        ui.addWarningLogMessage(_("No data selected"))#cn:没有选中数据
        return

    if not dadata.isDataFrame():
        ui.addWarningLogMessage(_("The data is not of DataFrame type"))#cn:数据不是DataFrame类型
        return
    
    df = dadata.toDataFrame()
    subset = None
    if col_selected_index and len(col_selected_index) > 0:
        subset = df.columns[col_selected_index]
    # 弹出通用设置界面，进行参数设置
    # how: {'any', 'all'}
    # thresh: int, optional
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Missing Value Removal Parameter Settings"))#cn:删除缺失值参数设置
    cfgBuilder.add_enum(
            name="how",
            display_name=_("Drop Method"),#cn:删除方法
            default_value="any",
            enum_items=["any", "all","none"],
            enum_descriptions=[_("If any NA values are present, drop that row"), _("If all values are NA, drop that row"),_("Not Specified")],#cn:任意一个Na值存在即删除，全部是Na值才删除,不指定
            description=_("Determine if row or column is removed from DataFrame, when we have at least one NA or all NA.")#cn:确定在 DataFrame 中存在至少一个缺失值（NA）或全部为缺失值时，需移除的维度是行还是列
        )
    cfgBuilder.add_int(
            name="thresh",
            display_name=_("thresh"),#cn:阈值
            default_value=0,
            min_value=0,
            max_value=1000,
            description=_("Require that many non-NA values. Cannot be combined with Drop Method.")#cn:要求指定非缺失值的数量，此参数如果设置非0，则删除方法需要指定为：不指定
        )
    cfgBuilder.add_bool(
            name="reindex",
            display_name=_("Reindex"), #cn:重建索引
            default_value=True,
            description=_("Whether to rebuild the index after drop na values")#cn:删除缺失值后，是否重建索引
        )
    config = ui.getConfigValues(cfgBuilder.to_json())
    if len(config) == 0:
        return
    how = config.get("how","any")
    thresh = config.get("thresh", 0)
    old_len = len(df)
    if thresh > 0:
        df.dropna(axis=0, subset=subset, thresh=thresh, inplace=True)
    else:
        if how == "none":
            how = None
        df.dropna(axis=0, subset=subset, how=how, inplace=True)
    reindex = config.get("reindex",True)
    if reindex:
        df.reset_index(drop=True, inplace=True)
    return old_len - len(df)