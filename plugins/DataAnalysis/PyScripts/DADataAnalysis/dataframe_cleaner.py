# -*- coding: utf-8 -*-
"""
DataFrame Data Cleaning Toolkit for Analysts
这个工具集为数据分析人员提供了全面的数据清理功能，具有直观的设置界面和详细的中文说明。
所有功能都设计为无需编程经验即可使用，通过可视化的参数设置对话框配置操作。
"""

import os
from typing import List, Dict, Optional, Any, Union, Tuple
import pandas as pd
from pathlib import Path
import numpy as np
import traceback
import threading
from loguru import logger
from DAWorkbench.da_logger import log_function_call  # type: ignore
import DAWorkbench.thread_status_manager as tsm
import DAWorkbench.property_config_builder as porpCfgBuilder
import da_app, da_interface, da_data

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

def _execute_dataframe_operation(dadata: da_data.DAData, 
                                 operation_name: str, 
                                 operation_func: callable,
                                 success_msg: str = None,
                                 is_object_persist = False) -> Optional[int]:
    """
    Execute DataFrame operations with standardized error handling
    执行DataFrame操作，包含标准化的错误处理
    """
    ui = da_app.getCore().getUiInterface()
    command = ui.getCommandInterface()
    
    try:
        old_df = dadata.toDataFrame()
        old_len = len(old_df)
        
        command.beginDataOperateCommand(
            data=dadata,
            text=operation_name,
            isObjectPersist=is_object_persist
        )
        
        df = operation_func(old_df)
        
        if df is None:
            return None
        
        dadata.setPyObject(df)
        command.endDataOperateCommand(data=dadata)
        
        new_len = len(df)
        changed_rows = abs(old_len - new_len)
        
        if success_msg:
            if "removed_rows" in success_msg:
                ui.addInfoLogMessage(success_msg.format(removed_rows=changed_rows))
            elif "changed_rows" in success_msg:
                ui.addInfoLogMessage(success_msg.format(changed_rows=changed_rows))
        
        return changed_rows
        
    except Exception as e:
        ui.addCriticalLogMessage(_("Operation failed: {error}").format(error=str(e)))  # cn: 操作失败
        logger.error(f"{operation_name} failed: {e}")
        traceback.print_exc()
        return None

# ============================================================================
# 基础清理功能
# ============================================================================

def dropna() -> Optional[int]:
    """
    Remove rows with missing (NaN) values
    删除包含缺失值（NaN）的行
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Remove Missing Values"))
    
    cfgBuilder.begin_group(_("Basic Settings"))  # cn: 基础设置
    cfgBuilder.add_enum(
        name="how",
        display_name=_("Removal Criteria"),  # cn: 删除条件
        default_value="any",
        enum_items=["any", "all"],
        enum_descriptions=[
            _("Remove row if ANY value is missing"),  # cn: 行中任意值为空时删除
            _("Remove row if ALL values are missing")  # cn: 行中所有值为空时删除
        ],
        description=_("Choose when to remove a row based on missing values")  # cn: 根据缺失值情况选择何时删除行
    )
    
    cfgBuilder.add_int(
        name="min_non_na",
        display_name=_("Minimum Non-NA Values"),  # cn: 最少非空值数量
        default_value=0,
        min_value=0,
        description=_("Minimum number of non-missing values required to keep the row (0=disable)")  # cn: 保留行所需的最少非缺失值数量（0=禁用）
    )
    cfgBuilder.end_group()
    
    cfgBuilder.begin_group(_("Advanced Settings"))  # cn: 高级设置
    cfgBuilder.add_bool(
        name="reindex",
        display_name=_("Reset Row Numbers"),  # cn: 重置行号
        default_value=True,
        description=_("Renumber rows from 0 after removing missing values")  # cn: 删除缺失值后从0开始重新编号行
    )
    cfgBuilder.end_group()
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.dropna")
    if len(config) == 0:
        return None
    
    how = config.get("how", "any")
    min_non_na = config.get("min_non_na", 0)
    reindex = config.get("reindex", True)
    
    def operation(original_df):
        try:
            if min_non_na > 0:
                df = original_df.dropna(axis=0, subset=subset, thresh=min_non_na)
            else:
                df = original_df.dropna(axis=0, subset=subset, how=how)
            
            if reindex:
                df.reset_index(drop=True, inplace=True)
            
            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to remove missing values: {error}").format(error=str(e)))  # cn: 删除缺失值失败
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Remove Missing Values"),  # cn: 删除缺失值
        operation,
        _("Removed {removed_rows} rows with missing values")  # cn: 删除了{removed_rows}个包含缺失值的行
    )

def drop_duplicates() -> Optional[int]:
    """
    Remove duplicate rows from the dataset
    从数据集中删除重复行
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Remove Duplicate Rows"))
    
    cfgBuilder.add_enum(
        name="keep",
        display_name=_("Which to Keep"),  # cn: 保留哪个
        default_value="first",
        enum_items=["first", "last", "none"],
        enum_descriptions=[
            _("Keep first occurrence, remove rest"),  # cn: 保留首次出现，删除其余
            _("Keep last occurrence, remove rest"),  # cn: 保留最后出现，删除其余
            _("Remove all duplicates")  # cn: 删除所有重复值
        ],
        description=_("When duplicates are found, decide which one to keep")  # cn: 发现重复值时，决定保留哪一个
    )
    
    cfgBuilder.add_bool(
        name="ignore_index",
        display_name=_("Reset Row Numbers"),  # cn: 重置行号
        default_value=True,
        description=_("Renumber rows starting from 0 after removing duplicates")  # cn: 删除重复值后从0开始重新编号行
    )
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.remove_duplicates")
    if len(config) == 0:
        return None
    
    keep = config.get("keep", "first")
    ignore_index = config.get("ignore_index", True)
    
    def operation(original_df):
        try:
            if keep == "none":
                keep_val = False
            else:
                keep_val = keep
            
            return original_df.drop_duplicates(subset=subset, keep=keep_val, ignore_index=ignore_index)
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to remove duplicates: {error}").format(error=str(e)))  # cn: 删除重复值失败
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Remove Duplicate Rows"),  # cn: 删除重复行
        operation,
        _("Removed {removed_rows} duplicate rows")  # cn: 删除了{removed_rows}个重复行
    )

def fillna() -> Optional[int]:
    """
    Fill missing values with various methods
    使用多种方法填充缺失值
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Fill Missing Values"))
    
    cfgBuilder.begin_group(_("Fill Method"))  # cn: 填充方法
    cfgBuilder.add_enum(
        name="method",
        display_name=_("Fill Method"),  # cn: 填充方法
        default_value="value",
        enum_items=["value", "forward", "backward", "mean", "median", "mode"],
        enum_descriptions=[
            _("Fill with a specific value"),  # cn: 用特定值填充
            _("Use previous non-missing value"),  # cn: 使用前一个非缺失值
            _("Use next non-missing value"),  # cn: 使用后一个非缺失值
            _("Fill with column average"),  # cn: 用列平均值填充
            _("Fill with column middle value"),  # cn: 用列中位数填充
            _("Fill with most common value")  # cn: 用最常出现的值填充
        ],
        description=_("Choose how to fill missing values")  # cn: 选择如何填充缺失值
    )
    cfgBuilder.end_group()
    
    cfgBuilder.begin_group(_("Fill Value Settings"))  # cn: 填充值设置
    cfgBuilder.add_double(
        name="value",
        display_name=_("Fill Value"),  # cn: 填充值
        default_value=0.0,
        description=_("Value to use when filling with specific value")  # cn: 使用特定值填充时使用的值
    )
    
    cfgBuilder.add_int(
        name="limit",
        display_name=_("Maximum Consecutive Fills"),  # cn: 最大连续填充数
        default_value=None,
        min_value=1,
        max_value=1000,
        description=_("Maximum number of consecutive missing values to fill (leave empty for unlimited)")  # cn: 要填充的连续缺失值的最大数量（留空表示无限制）
    )
    cfgBuilder.end_group()
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.fillna")
    if len(config) == 0:
        return None
    
    method = config.get("method", "value")
    fill_value = config.get("value", 0)
    limit = config.get("limit", None)
    
    def operation(original_df):
        try:
            df = original_df.copy()
            if subset:
                columns_to_fill = subset
            else:
                columns_to_fill = df.columns
            
            old_missing_count = df[columns_to_fill].isna().sum().sum()
            
            if method == "value":
                df[columns_to_fill] = df[columns_to_fill].fillna(fill_value, limit=limit)
            elif method == "forward":
                df[columns_to_fill] = df[columns_to_fill].fillna(method='ffill', limit=limit)
            elif method == "backward":
                df[columns_to_fill] = df[columns_to_fill].fillna(method='bfill', limit=limit)
            elif method == "mean":
                for col in columns_to_fill:
                    if pd.api.types.is_numeric_dtype(df[col]):
                        df[col] = df[col].fillna(df[col].mean(), limit=limit)
            elif method == "median":
                for col in columns_to_fill:
                    if pd.api.types.is_numeric_dtype(df[col]):
                        df[col] = df[col].fillna(df[col].median(), limit=limit)
            elif method == "mode":
                for col in columns_to_fill:
                    if not df[col].mode().empty:
                        df[col] = df[col].fillna(df[col].mode().iloc[0], limit=limit)
            
            new_missing_count = df[columns_to_fill].isna().sum().sum()
            filled_count = old_missing_count - new_missing_count
            
            if filled_count > 0:
                ui.addInfoLogMessage(_("Filled {count} missing values").format(count=filled_count))  # cn: 填充了{count}个缺失值
            
            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to fill missing values: {error}").format(error=str(e)))  # cn: 填充缺失值失败
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Fill Missing Values"),  # cn: 填充缺失值
        operation,
        ""
    )

def fill_interpolate() -> Optional[int]:
    """
    插值填充缺失值
    :return: 填充的行数
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    dadata, _df, subset = select_datas_info
    
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Interpolate Values Parameter Settings"))  # cn: 插值填充参数设置
    
    cfgBuilder.add_enum(
        name="method",
        display_name=_("Interpolation Method"),  # cn: 插值方法
        default_value="linear",
        enum_items=["linear", "time", "index", "pad", "nearest", "zero", "slinear", 
                   "quadratic", "cubic", "spline", "barycentric", "polynomial",
                   "krogh", "piecewise_polynomial", "pchip", "akima"],
        enum_descriptions=[
            _("Linear interpolation"),  # cn: 线性插值
            _("Time-based interpolation"),  # cn: 基于时间的插值
            _("Use index values"),  # cn: 使用索引值
            _("Pad interpolation"),  # cn: 填充插值
            _("Nearest interpolation"),  # cn: 最近邻插值
            _("Zero-order hold"),  # cn: 零阶保持
            _("Spline linear interpolation"),  # cn: 样条线性插值
            _("Quadratic interpolation"),  # cn: 二次插值
            _("Cubic interpolation"),  # cn: 三次插值
            _("Spline interpolation"),  # cn: 样条插值
            _("Barycentric interpolation"),  # cn: 重心插值
            _("Polynomial interpolation"),  # cn: 多项式插值
            _("Krogh interpolation"),  # cn: Krogh插值
            _("Piecewise polynomial interpolation"),  # cn: 分段多项式插值
            _("PCHIP interpolation"),  # cn: PCHIP插值
            _("Akima interpolation")  # cn: Akima插值
        ],
        description=_("Interpolation method to use")  # cn: 使用的插值方法
    )
    
    cfgBuilder.add_int(
        name="limit",
        display_name=_("Interpolation Limit"),  # cn: 插值限制
        default_value=None,
        min_value=1,
        max_value=10000,
        description=_("Maximum number of consecutive NaN values to interpolate")  # cn: 要插值的连续NaN值的最大数量
    )
    
    cfgBuilder.add_int(
        name="order",
        display_name=_("Spline Order"),  # cn: 样条阶数
        default_value=3,
        min_value=1,
        max_value=10,
        description=_("Order of the spline interpolator (used for 'spline' method)")  # cn: 样条插值的阶数（用于'spline'方法）
    )
    
    cfgBuilder.add_bool(
        name="inplace",
        display_name=_("Inplace Interpolation"),  # cn: 原地插值
        default_value=True,
        description=_("Whether to interpolate in place or return a copy")  # cn: 是否原地插值或返回副本
    )
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.interpolate")
    if len(config) == 0:
        return None
    
    method = config.get("method", "linear")
    limit = config.get("limit", None)
    order = config.get("order", 3)
    inplace = config.get("inplace", True)
    
    def operation(original_df):
        df = original_df.copy()
        if subset:
            columns_to_interpolate = subset
        else:
            columns_to_interpolate = df.columns
        
        # 计算插值前的缺失值数量
        old_missing_count = df[columns_to_interpolate].isna().sum().sum()
        
        # 为数值列执行插值
        for col in columns_to_interpolate:
            if pd.api.types.is_numeric_dtype(df[col]):
                try:
                    if method == "spline":
                        df[col] = df[col].interpolate(method=method, order=order, limit=limit, inplace=False)
                    else:
                        df[col] = df[col].interpolate(method=method, limit=limit, inplace=False)
                except Exception as e:
                    ui.addWarningLogMessage(_("Failed to interpolate column {col}: {error}").format(col=col, error=str(e)))  # cn: 插值列{col}失败
        
        # 计算插值后的缺失值数量
        new_missing_count = df[columns_to_interpolate].isna().sum().sum()
        
        return df, old_missing_count - new_missing_count
    
    return _execute_dataframe_operation(
        dadata,
        "Interpolate Missing Values",  # cn: 插值填充缺失值
        lambda df: operation(df)[0],
        "Interpolated {changed_rows} missing values",  # cn: 插值填充了{changed_rows}个缺失值
    )

def replace_specific_values() -> Optional[int]:
    """
    Replace specific values in the dataset
    替换数据集中的特定值
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Replace Values"))
    
    cfgBuilder.add_string(
        name="old_values",
        display_name=_("Values to Replace"),  # cn: 要替换的值
        default_value="",
        description=_("Values to find and replace (comma separated for multiple values)")  # cn: 要查找和替换的值（多个值用逗号分隔）
    )
    
    cfgBuilder.add_string(
        name="new_value",
        display_name=_("Replacement Value"),  # cn: 替换值
        default_value="",
        description=_("Value to use as replacement")  # cn: 用作替换的值
    )
    
    cfgBuilder.add_bool(
        name="case_sensitive",
        display_name=_("Case Sensitive"),  # cn: 区分大小写
        default_value=True,
        description=_("Consider letter case when matching values (for text only)")  # cn: 匹配值时考虑字母大小写（仅适用于文本）
    )
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.replace")
    if len(config) == 0:
        return None
    
    old_values_str = config.get("old_values", "")
    new_value = config.get("new_value", "")
    case_sensitive = config.get("case_sensitive", True)
    
    def operation(original_df):
        try:
            if not old_values_str:
                ui.addWarningLogMessage(_("Please specify values to replace"))  # cn: 请指定要替换的值
                return None
            
            old_values = [v.strip() for v in old_values_str.split(",") if v.strip()]
            
            if not old_values:
                ui.addWarningLogMessage(_("No valid values specified for replacement"))  # cn: 未指定有效的替换值
                return None
            df = original_df.copy()
            
            columns_to_check = subset if subset else df.columns
            replaced_count = 0
            
            for col in columns_to_check:
                for old_val in old_values:
                    try:
                        if pd.api.types.is_string_dtype(df[col]):
                            if case_sensitive:
                                mask = df[col] == old_val
                            else:
                                mask = df[col].astype(str).str.lower() == old_val.lower()
                        else:
                            # Try numeric conversion
                            try:
                                old_val_num = float(old_val)
                                mask = df[col] == old_val_num
                            except:
                                mask = df[col] == old_val
                        
                        count = mask.sum()
                        if count > 0:
                            df.loc[mask, col] = new_value
                            replaced_count += count
                            
                    except Exception as e:
                        ui.addWarningLogMessage(_("Error processing column '{col}': {error}").format(col=col, error=str(e)))  # cn: 处理列'{col}'时出错
            
            if replaced_count > 0:
                ui.addInfoLogMessage(_("Replaced {count} values").format(count=replaced_count))  # cn: 替换了{count}个值
            else:
                ui.addInfoLogMessage(_("No matching values found to replace"))  # cn: 未找到匹配的值进行替换
            
            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to replace values: {error}").format(error=str(e)))  # cn: 替换值失败
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Replace Values"),  # cn: 替换值
        operation,
        ""
    )
# ============================================================================
# 异常值处理（统一版本）
# ============================================================================

def remove_outliers_iqr() -> Optional[int]:
    """
    Remove or replace outliers using Interquartile Range (IQR) method
    使用四分位距（IQR）方法删除或替换异常值
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Remove Outliers (IQR Method)"))
    
    cfgBuilder.begin_group(_("Detection Settings"))  # cn: 检测设置
    cfgBuilder.add_double(
        name="multiplier",
        display_name=_("IQR Multiplier"),  # cn: IQR乘数
        default_value=1.5,
        min_value=0.5,
        max_value=10.0,
        description=_("Values beyond Q1-multiplier*IQR and Q3+multiplier*IQR are outliers (typical: 1.5-3)")  # cn: 超出Q1-乘数*IQR和Q3+乘数*IQR的值被视为异常值（典型值：1.5-3）
    )
    cfgBuilder.end_group()
    
    cfgBuilder.begin_group(_("Action Settings"))  # cn: 操作设置
    cfgBuilder.add_enum(
        name="action",
        display_name=_("Action on Outliers"),  # cn: 对异常值的操作
        default_value="remove",
        enum_items=["remove", "replace_mean", "replace_median", "replace_boundary", "replace_custom"],
        enum_descriptions=[
            _("Remove rows with outliers"),  # cn: 删除包含异常值的行
            _("Replace outliers with column mean"),  # cn: 用列平均值替换异常值
            _("Replace outliers with column median"),  # cn: 用列中位数替换异常值
            _("Replace outliers with boundary values"),  # cn: 用边界值替换异常值
            _("Replace outliers with custom value")  # cn: 用自定义值替换异常值
        ],
        description=_("How to handle detected outliers")  # cn: 如何处理检测到的异常值
    )
    
    cfgBuilder.add_double(
        name="custom_value",
        display_name=_("Custom Replacement Value"),  # cn: 自定义替换值
        default_value=0.0,
        description=_("Value to use for replacement when action is 'replace_custom'")  # cn: 当操作为'replace_custom'时使用的替换值
    )
    
    cfgBuilder.add_bool(
        name="reindex",
        display_name=_("Reset Row Numbers"),  # cn: 重置行号
        default_value=True,
        description=_("Renumber rows after removing outliers (only for 'remove' action)")  # cn: 删除异常值后重新编号行（仅适用于'remove'操作）
    )
    cfgBuilder.end_group()
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.remove_outliers_iqr")
    if len(config) == 0:
        return None
    
    multiplier = config.get("multiplier", 1.5)
    action = config.get("action", "remove")
    custom_value = config.get("custom_value", 0.0)
    reindex = config.get("reindex", True)
    
    def operation(original_df):
        try:
            if not subset:
                # If no columns selected, use all numeric columns
                # 如果没有选择列，使用所有数值列
                numeric_cols = original_df.select_dtypes(include=[np.number]).columns.tolist()
                if not numeric_cols:
                    ui.addWarningLogMessage(_("No numeric columns found for outlier detection"))  # cn: 未找到用于异常值检测的数值列
                    return None
                columns_to_check = numeric_cols
            else:
                # Only check selected numeric columns
                # 只检查选中的数值列
                columns_to_check = [col for col in subset if pd.api.types.is_numeric_dtype(original_df[col])]
            df = original_df.copy()
            old_len = len(df)
            total_outliers = 0
            
            if action == "remove":
                # Create mask for rows to keep (initially all True)
                # 创建要保留的行的掩码（初始全部为True）
                mask = pd.Series([True] * len(df))
                
                for col in columns_to_check:
                    try:
                        # Calculate IQR boundaries
                        # 计算IQR边界
                        Q1 = df[col].quantile(0.25)
                        Q3 = df[col].quantile(0.75)
                        IQR = Q3 - Q1
                        lower_bound = Q1 - multiplier * IQR
                        upper_bound = Q3 + multiplier * IQR
                        
                        # Identify outliers
                        # 识别异常值
                        outliers_mask = (df[col] < lower_bound) | (df[col] > upper_bound)
                        outlier_count = outliers_mask.sum()
                        
                        if outlier_count > 0:
                            # Keep rows that are NOT outliers in this column
                            # 保留此列中不是异常值的行
                            col_mask = ~outliers_mask
                            mask = mask & col_mask
                            total_outliers += outlier_count
                            
                    except Exception as e:
                        ui.addWarningLogMessage(_("Error processing column '{col}': {error}").format(col=col, error=str(e)))  # cn: 处理列'{col}'时出错
                
                # Remove rows with outliers
                # 删除包含异常值的行
                df = df[mask]
                
                if reindex:
                    df = df.reset_index(drop=True)
                    
            else:
                # Replace outliers instead of removing
                # 替换异常值而不是删除
                for col in columns_to_check:
                    try:
                        # Calculate IQR boundaries
                        # 计算IQR边界
                        Q1 = df[col].quantile(0.25)
                        Q3 = df[col].quantile(0.75)
                        IQR = Q3 - Q1
                        lower_bound = Q1 - multiplier * IQR
                        upper_bound = Q3 + multiplier * IQR
                        
                        # Identify outliers
                        # 识别异常值
                        outliers_mask = (df[col] < lower_bound) | (df[col] > upper_bound)
                        outlier_count = outliers_mask.sum()
                        
                        if outlier_count > 0:
                            total_outliers += outlier_count
                            
                            # Determine replacement value
                            # 确定替换值
                            if action == "replace_mean":
                                replacement = df[col].mean()
                            elif action == "replace_median":
                                replacement = df[col].median()
                            elif action == "replace_boundary":
                                # Replace with boundary value (clip to IQR bounds)
                                # 用边界值替换（裁剪到IQR边界）
                                # For upper outliers, replace with upper bound; for lower outliers, replace with lower bound
                                # 上界异常值用上界替换，下界异常值用下界替换
                                df.loc[outliers_mask & (df[col] > upper_bound), col] = upper_bound
                                df.loc[outliers_mask & (df[col] < lower_bound), col] = lower_bound
                                continue  # Skip the general replacement below
                                
                            elif action == "replace_custom":
                                replacement = custom_value
                            
                            # Replace outliers with the determined value
                            # 用确定的值替换异常值
                            df.loc[outliers_mask, col] = replacement
                            
                    except Exception as e:
                        ui.addWarningLogMessage(_("Error processing column '{col}': {error}").format(col=col, error=str(e)))  # cn: 处理列'{col}'时出错
            
            # Provide feedback to user
            # 向用户提供反馈
            if total_outliers > 0:
                if action == "remove":
                    removed_rows = old_len - len(df)
                    ui.addInfoLogMessage(_("Removed {count} rows containing outliers using IQR method").format(count=removed_rows))  # cn: 使用IQR方法删除了{count}个包含异常值的行
                else:
                    action_descriptions = {
                        "replace_mean": _("replaced with mean"),  # cn: 用平均值替换
                        "replace_median": _("replaced with median"),  # cn: 用中位数替换
                        "replace_boundary": _("replaced with boundary values"),  # cn: 用边界值替换
                        "replace_custom": _("replaced with custom value")  # cn: 用自定义值替换
                    }
                    ui.addInfoLogMessage(_("Replaced {count} outlier values using IQR method ({action})").format(
                        count=total_outliers, action=action_descriptions.get(action, action)
                    ))  # cn: 使用IQR方法替换了{count}个异常值（{action}）
            else:
                ui.addInfoLogMessage(_("No outliers detected using IQR method"))  # cn: 使用IQR方法未检测到异常值
            
            return df
            
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to process outliers: {error}").format(error=str(e)))  # cn: 处理异常值失败
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Remove Outliers (IQR)"),  # cn: 移除异常值（IQR）
        operation,
        _("Removed {removed_rows} rows outliers")  # cn: 处理了{removed_rows}行异常值
    )

def remove_outliers_zscore() -> Optional[int]:
    """
    Remove or replace outliers using Z-score method
    使用Z-score方法删除或替换异常值
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Remove Outliers (Z-score)"))
    
    cfgBuilder.begin_group(_("Detection Settings"))  # cn: 检测设置
    cfgBuilder.add_double(
        name="threshold",
        display_name=_("Z-score Threshold"),  # cn: Z-score阈值
        default_value=3.0,
        min_value=1.0,
        max_value=10.0,
        description=_("Values with absolute Z-score above this threshold are outliers (typical: 3)")  # cn: 绝对值超过此阈值的Z-score被视为异常值（典型值：3）
    )
    
    cfgBuilder.add_bool(
        name="robust",
        display_name=_("Use Robust Statistics"),  # cn: 使用稳健统计
        default_value=False,
        description=_("Use median and MAD instead of mean and std (better for non-normal data)")  # cn: 使用中位数和MAD代替均值和标准差（适用于非正态数据）
    )
    cfgBuilder.end_group()
    
    cfgBuilder.begin_group(_("Action Settings"))  # cn: 操作设置
    cfgBuilder.add_enum(
        name="action",
        display_name=_("Action on Outliers"),  # cn: 对异常值的操作
        default_value="remove",
        enum_items=["remove", "replace_mean", "replace_median", "replace_boundary", "replace_custom"],
        enum_descriptions=[
            _("Remove rows with outliers"),  # cn: 删除包含异常值的行
            _("Replace outliers with column mean"),  # cn: 用列平均值替换异常值
            _("Replace outliers with column median"),  # cn: 用列中位数替换异常值
            _("Replace outliers with boundary values"),  # cn: 用边界值替换异常值
            _("Replace outliers with custom value")  # cn: 用自定义值替换异常值
        ],
        description=_("How to handle detected outliers")  # cn: 如何处理检测到的异常值
    )
    
    cfgBuilder.add_double(
        name="custom_value",
        display_name=_("Custom Replacement Value"),  # cn: 自定义替换值
        default_value=0.0,
        description=_("Value to use for replacement when action is 'replace_custom'")  # cn: 当操作为'replace_custom'时使用的替换值
    )
    
    cfgBuilder.add_bool(
        name="reindex",
        display_name=_("Reset Row Numbers"),  # cn: 重置行号
        default_value=True,
        description=_("Renumber rows after removing outliers (only for 'remove' action)")  # cn: 删除异常值后重新编号行（仅适用于'remove'操作）
    )
    cfgBuilder.end_group()
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.remove_outliers_zscore")
    if len(config) == 0:
        return None
    
    threshold = config.get("threshold", 3.0)
    robust = config.get("robust", False)
    action = config.get("action", "remove")
    custom_value = config.get("custom_value", 0.0)
    reindex = config.get("reindex", True)
    
    def operation(original_df):
        try:
            if not subset:
                # If no columns selected, use all numeric columns
                # 如果没有选择列，使用所有数值列
                numeric_cols = original_df.select_dtypes(include=[np.number]).columns.tolist()
                if not numeric_cols:
                    ui.addWarningLogMessage(_("No numeric columns found for outlier detection"))  # cn: 未找到用于异常值检测的数值列
                    return None
                columns_to_check = numeric_cols
            else:
                # Only check selected numeric columns
                # 只检查选中的数值列
                columns_to_check = [col for col in subset if pd.api.types.is_numeric_dtype(df[col])]
            df = original_df.copy()
            old_len = len(df)
            total_outliers = 0
            
            if action == "remove":
                # Create mask for rows to keep (initially all True)
                # 创建要保留的行的掩码（初始全部为True）
                mask = pd.Series([True] * len(df))
                
                for col in columns_to_check:
                    try:
                        # Calculate Z-scores
                        # 计算Z分数
                        if robust:
                            median = df[col].median()
                            mad = np.median(np.abs(df[col] - median))
                            std_estimate = mad * 1.4826 if mad != 0 else 1e-10
                            z_scores = (df[col] - median) / std_estimate
                        else:
                            mean = df[col].mean()
                            std = df[col].std()
                            std = std if std != 0 else 1e-10
                            z_scores = (df[col] - mean) / std
                        
                        # Identify outliers
                        # 识别异常值
                        outliers_mask = np.abs(z_scores) > threshold
                        outlier_count = outliers_mask.sum()
                        
                        if outlier_count > 0:
                            # Keep rows that are NOT outliers in this column
                            # 保留此列中不是异常值的行
                            col_mask = ~outliers_mask
                            mask = mask & col_mask
                            total_outliers += outlier_count
                            
                    except Exception as e:
                        ui.addWarningLogMessage(_("Error processing column '{col}': {error}").format(col=col, error=str(e)))  # cn: 处理列'{col}'时出错
                
                # Remove rows with outliers
                # 删除包含异常值的行
                df = df[mask]
                
                if reindex:
                    df = df.reset_index(drop=True)
                    
            else:
                # Replace outliers instead of removing
                # 替换异常值而不是删除
                for col in columns_to_check:
                    try:
                        # Calculate Z-scores
                        # 计算Z分数
                        if robust:
                            median = df[col].median()
                            mad = np.median(np.abs(df[col] - median))
                            std_estimate = mad * 1.4826 if mad != 0 else 1e-10
                            z_scores = (df[col] - median) / std_estimate
                        else:
                            mean = df[col].mean()
                            std = df[col].std()
                            std = std if std != 0 else 1e-10
                            z_scores = (df[col] - mean) / std
                        
                        # Identify outliers
                        # 识别异常值
                        outliers_mask = np.abs(z_scores) > threshold
                        outlier_count = outliers_mask.sum()
                        
                        if outlier_count > 0:
                            total_outliers += outlier_count
                            
                            # Determine replacement value
                            # 确定替换值
                            if action == "replace_mean":
                                replacement = df[col].mean()
                            elif action == "replace_median":
                                replacement = df[col].median()
                            elif action == "replace_boundary":
                                # Replace with boundary value (mean ± threshold*std or median ± threshold*mad)
                                # 用边界值替换（平均值 ± 阈值*标准差 或 中位数 ± 阈值*MAD）
                                if robust:
                                    median = df[col].median()
                                    mad = np.median(np.abs(df[col] - median))
                                    std_estimate = mad * 1.4826 if mad != 0 else 1e-10
                                    upper_bound = median + threshold * std_estimate
                                    lower_bound = median - threshold * std_estimate
                                else:
                                    mean = df[col].mean()
                                    std = df[col].std()
                                    upper_bound = mean + threshold * std
                                    lower_bound = mean - threshold * std
                                
                                # Replace upper outliers with upper bound, lower outliers with lower bound
                                # 上界异常值用上界替换，下界异常值用下界替换
                                df.loc[outliers_mask & (df[col] > (median if robust else mean)), col] = upper_bound
                                df.loc[outliers_mask & (df[col] < (median if robust else mean)), col] = lower_bound
                                continue  # Skip the general replacement below
                                
                            elif action == "replace_custom":
                                replacement = custom_value
                            
                            # Replace outliers with the determined value
                            # 用确定的值替换异常值
                            df.loc[outliers_mask, col] = replacement
                            
                    except Exception as e:
                        ui.addWarningLogMessage(_("Error processing column '{col}': {error}").format(col=col, error=str(e)))  # cn: 处理列'{col}'时出错
            
            # Provide feedback to user
            # 向用户提供反馈
            if total_outliers > 0:
                if action == "remove":
                    removed_rows = old_len - len(df)
                    ui.addInfoLogMessage(_("Removed {count} rows containing outliers using Z-score method").format(count=removed_rows))  # cn: 使用Z-score方法删除了{count}个包含异常值的行
                else:
                    action_descriptions = {
                        "replace_mean": _("replaced with mean"),  # cn: 用平均值替换
                        "replace_median": _("replaced with median"),  # cn: 用中位数替换
                        "replace_boundary": _("replaced with boundary values"),  # cn: 用边界值替换
                        "replace_custom": _("replaced with custom value")  # cn: 用自定义值替换
                    }
                    ui.addInfoLogMessage(_("Replaced {count} outlier values using Z-score method ({action})").format(
                        count=total_outliers, action=action_descriptions.get(action, action)
                    ))  # cn: 使用Z-score方法替换了{count}个异常值（{action}）
            else:
                ui.addInfoLogMessage(_("No outliers detected using Z-score method"))  # cn: 使用Z-score方法未检测到异常值
            
            return df
            
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to process outliers: {error}").format(error=str(e)))  # cn: 处理异常值失败
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Remove Outliers (Z-Score)"),  # cn: 移除异常值（Z-Score）
        operation,
        _("Removed {removed_rows} rows outliers")  # cn: 处理了{removed_rows}行异常值
    )
# ============================================================================
# 数据转换（简化设置）
# ============================================================================

def transform_skewed_data() -> Optional[int]:
    """
    Transform skewed numerical data to improve distribution
    转换偏态数值数据以改善分布
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Transform Skewed Data"))
    
    cfgBuilder.add_enum(
        name="method",
        display_name=_("Transformation Method"),  # cn: 转换方法
        default_value="log",
        enum_items=["log", "sqrt", "reciprocal", "power"],
        enum_descriptions=[
            _("Logarithm transformation (log(x))"),  # cn: 对数转换（log(x)）
            _("Square root transformation (sqrt(x))"),  # cn: 平方根转换（sqrt(x)）
            _("Reciprocal transformation (1/x)"),  # cn: 倒数转换（1/x）
            _("Power transformation (x^lambda)")  # cn: 幂转换（x^lambda）
        ],
        description=_("Choose transformation method for reducing skewness")  # cn: 选择减少偏度的转换方法
    )
    
    cfgBuilder.add_double(
        name="lambda_value",
        display_name=_("Power Lambda"),  # cn: 幂Lambda值
        default_value=0.5,
        min_value=-5.0,
        max_value=5.0,
        description=_("Power value for power transformation (ignored for other methods)")  # cn: 幂转换的指数值（其他方法忽略此值）
    )
    
    cfgBuilder.add_bool(
        name="add_one",
        display_name=_("Add 1 before transformation"),  # cn: 转换前加1
        default_value=True,
        description=_("Add 1 to values before log/sqrt transformation (handles zeros)")  # cn: 在对数/平方根转换前加1（处理零值）
    )
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.transform_skewed")
    if len(config) == 0:
        return None
    
    method = config.get("method", "log")
    lambda_value = config.get("lambda_value", 0.5)
    add_one = config.get("add_one", True)
    
    def operation(original_df):
        try:
            if not subset:
                numeric_cols = original_df.select_dtypes(include=[np.number]).columns.tolist()
                if not numeric_cols:
                    ui.addWarningLogMessage(_("No numeric columns found for transformation"))  # cn: 未找到用于转换的数值列
                    return None
                columns_to_transform = numeric_cols
            else:
                columns_to_transform = [col for col in subset if pd.api.types.is_numeric_dtype(original_df[col])]
            df = original_df.copy()
            for col in columns_to_transform:
                try:
                    col_data = df[col].copy()
                    
                    if method == "log":
                        if add_one:
                            df[col] = np.log1p(col_data.where(col_data >= 0, np.nan))
                        else:
                            df[col] = np.log(col_data.where(col_data > 0, np.nan))
                    
                    elif method == "sqrt":
                        if add_one:
                            df[col] = np.sqrt(col_data.where(col_data >= -1, np.nan) + 1)
                        else:
                            df[col] = np.sqrt(col_data.where(col_data >= 0, np.nan))
                    
                    elif method == "reciprocal":
                        df[col] = 1 / col_data.where(col_data != 0, np.nan)
                    
                    elif method == "power":
                        df[col] = col_data ** lambda_value
                    
                    ui.addInfoLogMessage(_("Transformed column: {col}").format(col=col))  # cn: 已转换列
                    
                except Exception as e:
                    ui.addWarningLogMessage(_("Error transforming column '{col}': {error}").format(col=col, error=str(e)))  # cn: 转换列'{col}'时出错
            
            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to transform data: {error}").format(error=str(e)))  # cn: 转换数据失败
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Transform Skewed Data"),  # cn: 转换偏态数据
        operation,
        _("Transformed {changed_rows} columns"),  # cn: 转换了{changed_rows}列
    )
def scale_data() -> Optional[int]:
    """
    数据缩放（标准化/归一化）
    :return: 缩放的列数
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    dadata, _df, subset = select_datas_info
    
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Scale Data Parameter Settings"))  # cn: 数据缩放参数设置
    
    cfgBuilder.add_enum(
        name="scaling_method",
        display_name=_("Scaling Method"),  # cn: 缩放方法
        default_value="standard",
        enum_items=["standard", "minmax", "robust", "maxabs", "unit_vector"],
        enum_descriptions=[
            _("Standard scaling (z-score: (x-mean)/std)"),  # cn: 标准缩放（z-score：(x-均值)/标准差）
            _("Min-Max scaling (to [0, 1] range)"),  # cn: 最小-最大缩放（到[0, 1]范围）
            _("Robust scaling (using median and IQR)"),  # cn: 稳健缩放（使用中位数和IQR）
            _("MaxAbs scaling (to [-1, 1] range)"),  # cn: 最大绝对值缩放（到[-1, 1]范围）
            _("Unit vector scaling (norm=1)")  # cn: 单位向量缩放（范数=1）
        ],
        description=_("Method for scaling data")  # cn: 数据缩放方法
    )
    
    cfgBuilder.add_double(
        name="feature_range_min",
        display_name=_("Feature Range (Min)"),  # cn: 特征范围（最小值）
        default_value=0.0,
        description=_("Minimum value for min-max scaling range")  # cn: 最小-最大缩放范围的最小值
    )
    
    cfgBuilder.add_double(
        name="feature_range_max",
        display_name=_("Feature Range (Max)"),  # cn: 特征范围（最大值）
        default_value=1.0,
        description=_("Maximum value for min-max scaling range")  # cn: 最小-最大缩放范围的最大值
    )
    
    cfgBuilder.add_bool(
        name="with_mean",
        display_name=_("With Mean (for standard scaling)"),  # cn: 使用均值（用于标准缩放）
        default_value=True,
        description=_("Whether to center the data before scaling (for standard scaling)")  # cn: 是否在缩放前将数据中心化（用于标准缩放）
    )
    
    cfgBuilder.add_bool(
        name="with_std",
        display_name=_("With Std (for standard scaling)"),  # cn: 使用标准差（用于标准缩放）
        default_value=True,
        description=_("Whether to scale to unit variance (for standard scaling)")  # cn: 是否缩放到单位方差（用于标准缩放）
    )
    
    cfgBuilder.add_bool(
        name="create_new_columns",
        display_name=_("Create New Columns"),  # cn: 创建新列
        default_value=True,
        description=_("Create new columns instead of replacing original data")  # cn: 创建新列而不是替换原始数据
    )
    
    cfgBuilder.add_string(
        name="suffix",
        display_name=_("Column Suffix"),  # cn: 列后缀
        default_value="_scaled",
        description=_("Suffix for new column names")  # cn: 新列名的后缀
    )
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.scale_data")
    if len(config) == 0:
        return None
    
    scaling_method = config.get("scaling_method", "standard")
    feature_range_min = config.get("feature_range_min", 0.0)
    feature_range_max = config.get("feature_range_max", 1.0)
    with_mean = config.get("with_mean", True)
    with_std = config.get("with_std", True)
    create_new_columns = config.get("create_new_columns", True)
    suffix = config.get("suffix", "_scaled")
    
    def operation(original_df):
        if not subset:
            # 如果没有选择列，使用所有数值列
            numeric_cols = original_df.select_dtypes(include=[np.number]).columns.tolist()
            if not numeric_cols:
                ui.addWarningLogMessage(_("No numeric columns found for scaling"))  # cn: 未找到用于缩放的数值列
                return None, 0
            columns_to_scale = numeric_cols
        else:
            # 只缩放选中的数值列
            columns_to_scale = [col for col in subset if pd.api.types.is_numeric_dtype(original_df[col])]
        df = original_df.copy()
        scaled_count = 0
        
        for col in columns_to_scale:
            try:
                col_data = df[col].copy()
                
                if scaling_method == "standard":
                    # 标准缩放 (z-score)
                    if with_mean:
                        mean = col_data.mean()
                    else:
                        mean = 0
                    
                    if with_std:
                        std = col_data.std()
                        std = std if std != 0 else 1e-10
                    else:
                        std = 1
                    
                    scaled = (col_data - mean) / std
                
                elif scaling_method == "minmax":
                    # 最小-最大缩放
                    col_min = col_data.min()
                    col_max = col_data.max()
                    
                    if col_max != col_min:
                        scaled = (col_data - col_min) / (col_max - col_min)
                        # 缩放到指定范围
                        scaled = scaled * (feature_range_max - feature_range_min) + feature_range_min
                    else:
                        scaled = pd.Series([feature_range_min] * len(col_data), dtype=float)
                
                elif scaling_method == "robust":
                    # 稳健缩放
                    median = col_data.median()
                    q1 = col_data.quantile(0.25)
                    q3 = col_data.quantile(0.75)
                    iqr = q3 - q1
                    iqr = iqr if iqr != 0 else 1e-10
                    
                    scaled = (col_data - median) / iqr
                
                elif scaling_method == "maxabs":
                    # 最大绝对值缩放
                    max_abs = np.abs(col_data).max()
                    max_abs = max_abs if max_abs != 0 else 1e-10
                    
                    scaled = col_data / max_abs
                
                elif scaling_method == "unit_vector":
                    # 单位向量缩放
                    norm = np.sqrt((col_data ** 2).sum())
                    norm = norm if norm != 0 else 1e-10
                    
                    scaled = col_data / norm
                
                # 应用缩放
                if create_new_columns:
                    new_col_name = f"{col}{suffix}"
                    df[new_col_name] = scaled
                else:
                    df[col] = scaled
                
                scaled_count += 1
                
            except Exception as e:
                ui.addWarningLogMessage(_("Failed to scale column {col}: {error}").format(col=col, error=str(e)))  # cn: 缩放列{col}失败
        
        return df, scaled_count
    
    return _execute_dataframe_operation(
        dadata,
        _("Scale Data"),  # cn: 数据缩放
        lambda df: operation(df)[0],
        _("Scaled {changed_rows} columns")  # cn: 缩放{changed_rows}列
    )

# ============================================================================
# 文本处理（简化）
# ============================================================================

def clean_text_strings() -> Optional[int]:
    """
    Clean and standardize text strings
    清理和标准化文本字符串
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Clean Text Strings"))
    
    cfgBuilder.add_enum(
        name="trim_type",
        display_name=_("Trim Spaces"),  # cn: 修剪空格
        default_value="both",
        enum_items=["both", "left", "right", "none"],
        enum_descriptions=[
            _("Remove spaces from both sides"),  # cn: 删除两侧空格
            _("Remove spaces from left side only"),  # cn: 仅删除左侧空格
            _("Remove spaces from right side only"),  # cn: 仅删除右侧空格
            _("Do not trim spaces")  # cn: 不修剪空格
        ],
        description=_("Remove leading/trailing spaces from text")  # cn: 从文本中删除前导/尾随空格
    )
    
    cfgBuilder.add_bool(
        name="lowercase",
        display_name=_("Convert to Lowercase"),  # cn: 转换为小写
        default_value=False,
        description=_("Convert all text to lowercase letters")  # cn: 将所有文本转换为小写字母
    )
    
    cfgBuilder.add_bool(
        name="remove_extra_spaces",
        display_name=_("Remove Extra Spaces"),  # cn: 删除额外空格
        default_value=True,
        description=_("Replace multiple spaces with single space")  # cn: 将多个空格替换为单个空格
    )
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.clean_text")
    if len(config) == 0:
        return None
    
    trim_type = config.get("trim_type", "both")
    lowercase = config.get("lowercase", False)
    remove_extra_spaces = config.get("remove_extra_spaces", True)
    
    def operation(original_df):
        try:
            if subset:
                text_cols = [col for col in subset if original_df[col].dtype == 'object']
            else:
                text_cols = original_df.select_dtypes(include=['object']).columns.tolist()
            
            if not text_cols:
                ui.addWarningLogMessage(_("No text columns found for cleaning"))  # cn: 未找到用于清理的文本列
                return None
            df = original_df.copy()
            for col in text_cols:
                try:
                    # Trim spaces
                    if trim_type == "both":
                        df[col] = df[col].str.strip()
                    elif trim_type == "left":
                        df[col] = df[col].str.lstrip()
                    elif trim_type == "right":
                        df[col] = df[col].str.rstrip()
                    
                    # Remove extra spaces
                    if remove_extra_spaces:
                        df[col] = df[col].str.replace(r'\s+', ' ', regex=True)
                    
                    # Convert to lowercase
                    if lowercase:
                        df[col] = df[col].str.lower()
                    
                except Exception as e:
                    ui.addWarningLogMessage(_("Error cleaning column '{col}': {error}").format(col=col, error=str(e)))  # cn: 清理列'{col}'时出错
            
            ui.addInfoLogMessage(_("Cleaned {count} text columns").format(count=len(text_cols)))  # cn: 清理了{count}个文本列
            
            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to clean text: {error}").format(error=str(e)))  # cn: 清理文本失败
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Clean Text Strings"),  # cn: 清理文本字符串
        operation,
        ""
    )

# ============================================================================
# 新增：专用编码函数（分离复杂功能）
# ============================================================================

def encode_label() -> Optional[int]:
    """
    Convert categorical text to numeric labels (0, 1, 2, ...)
    将分类文本转换为数字标签（0, 1, 2, ...）
    """
    select_datas_info = get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfgBuilder = porpCfgBuilder.PropertyConfigBuilder(_("Label Encoding"))
    
    cfgBuilder.add_bool(
        name="keep_original",
        display_name=_("Keep Original Column"),  # cn: 保留原始列
        default_value=True,
        description=_("Keep original text column and create new encoded column")  # cn: 保留原始文本列并创建新的编码列
    )
    
    config = ui.getConfigValues(cfgBuilder.to_json(), "dataframecleaner.encode_label")
    if len(config) == 0:
        return None
    
    keep_original = config.get("keep_original", True)
    
    def operation(original_df):
        try:
            if subset:
                text_cols = [col for col in subset if original_df[col].dtype == 'object']
            else:
                text_cols = original_df.select_dtypes(include=['object']).columns.tolist()
            
            if not text_cols:
                ui.addWarningLogMessage(_("No text columns found for label encoding"))  # cn: 未找到用于标签编码的文本列
                return None
            df = original_df.copy()
            for col in text_cols:
                try:
                    unique_values = df[col].unique()
                    value_mapping = {val: i for i, val in enumerate(unique_values)}
                    
                    if keep_original:
                        df[f"{col}_encoded"] = df[col].map(value_mapping)
                    else:
                        df[col] = df[col].map(value_mapping)
                    
                    ui.addInfoLogMessage(_("Encoded '{col}' with {count} unique values").format(col=col, count=len(unique_values)))  # cn: 对'{col}'进行了编码，共{count}个唯一值
                    
                except Exception as e:
                    ui.addWarningLogMessage(_("Error encoding column '{col}': {error}").format(col=col, error=str(e)))  # cn: 编码列'{col}'时出错
            
            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to encode labels: {error}").format(error=str(e)))  # cn: 标签编码失败
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Label Encoding"),  # cn: 标签编码
        operation,
        ""
    )
