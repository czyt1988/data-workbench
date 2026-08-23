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
from DAWorkbench.DAPyBase.da_logger import log_function_call  # type: ignore
import DAWorkbench.DAPyBase.thread_status_manager as tsm
from DAWorkbench.DAPyBase.form_builder import FormBuilder, option
from DADataAnalysisGui import utils
from DADataAnalysisCore import cleaning as core_cleaning
import da_app, da_interface, da_data


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
        ui.addCriticalLogMessage(_("Operation failed: {error}").format(error=str(e)))  # cn: 操作失败：{error}
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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Remove Missing Values"))
        .group("basic", _("Basic Settings"))  # cn: 基础设置
        .enum("how", label=_("Removal Criteria"),  # cn: 删除条件
              default="any",
              options=[
                  option("any", _("Remove row if ANY value is missing")),  # cn: 行中任意值为空时删除
                  option("all", _("Remove row if ALL values are missing")),  # cn: 行中所有值为空时删除
              ],
              description=_("Choose when to remove a row based on missing values"))  # cn: 根据缺失值情况选择何时删除行
        .int("min_non_na", label=_("Minimum Non-NA Values"),  # cn: 最少非空值数量
             default=0, min=0,
             description=_("Minimum number of non-missing values required to keep the row (0=disable)"))  # cn: 保留行所需的最少非缺失值数量（0=禁用）
        .end_group()
        .group("advanced", _("Advanced Settings"))  # cn: 高级设置
        .bool("reindex", label=_("Reset Row Numbers"),  # cn: 重置行号
              default=True,
              description=_("Renumber rows from 0 after removing missing values"))  # cn: 删除缺失值后从0开始重新编号行
        .end_group()
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.dropna")
    if len(config) == 0:
        return None
    
    how = config.get("how", "any")
    min_non_na = config.get("min_non_na", 0)
    reindex = config.get("reindex", True)
    
    def operation(original_df):
        try:
            return core_cleaning.dropna_impl(original_df, subset=subset, how=how,
                                              min_non_na=min_non_na, reindex=reindex)
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to remove missing values: {error}").format(error=str(e)))  # cn: 移除缺失值失败：{error}
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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Remove Duplicate Rows"))
        .enum("keep", label=_("Which to Keep"),  # cn: 保留哪个
              default="first",
              options=[
                  option("first", _("Keep first occurrence, remove rest")),  # cn: 保留首次出现，删除其余
                  option("last", _("Keep last occurrence, remove rest")),  # cn: 保留最后出现，删除其余
                  option("none", _("Remove all duplicates")),  # cn: 删除所有重复值
              ],
              description=_("When duplicates are found, decide which one to keep"))  # cn: 发现重复值时，决定保留哪一个
        .bool("ignore_index", label=_("Reset Row Numbers"),  # cn: 重置行号
              default=True,
              description=_("Renumber rows starting from 0 after removing duplicates"))  # cn: 删除重复值后从0开始重新编号行
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.remove_duplicates")
    if len(config) == 0:
        return None
    
    keep = config.get("keep", "first")
    ignore_index = config.get("ignore_index", True)
    
    def operation(original_df):
        try:
            return core_cleaning.drop_duplicates_impl(original_df, subset=subset, keep=keep,
                                                       ignore_index=ignore_index)
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to remove duplicates: {error}").format(error=str(e)))  # cn: 移除重复行失败：{error}
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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Fill Missing Values"))
        .group("fill_method", _("Fill Method"))  # cn: 填充方法
        .enum("method", label=_("Fill Method"),  # cn: 填充方法
              default="value",
              options=[
                  option("value", _("Fill with a specific value")),  # cn: 用特定值填充
                  option("forward", _("Use previous non-missing value")),  # cn: 使用前一个非缺失值
                  option("backward", _("Use next non-missing value")),  # cn: 使用后一个非缺失值
                  option("mean", _("Fill with column average")),  # cn: 用列平均值填充
                  option("median", _("Fill with column middle value")),  # cn: 用列中位数填充
                  option("mode", _("Fill with most common value")),  # cn: 用最常出现的值填充
              ],
              description=_("Choose how to fill missing values"))  # cn: 选择如何填充缺失值
        .end_group()
        .group("fill_value", _("Fill Value Settings"))  # cn: 填充值设置
        .float("value", label=_("Fill Value"),  # cn: 填充值
               default=0.0,
               description=_("Value to use when filling with specific value"))  # cn: 使用特定值填充时使用的值
        .int("limit", label=_("Maximum Consecutive Fills"),  # cn: 最大连续填充数
             default=None, min=1, max=1000,
             description=_("Maximum number of consecutive missing values to fill (leave empty for unlimited)"))  # cn: 要填充的连续缺失值的最大数量（留空表示无限制）
        .end_group()
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.fillna")
    if len(config) == 0:
        return None
    
    method = config.get("method", "value")
    fill_value = config.get("value", 0)
    limit = config.get("limit", None)
    
    def operation(original_df):
        try:
            columns_to_fill = subset if subset else original_df.columns.tolist()
            old_missing_count = original_df[columns_to_fill].isna().sum().sum()

            df = core_cleaning.fillna_impl(original_df, subset=subset, method=method,
                                            value=fill_value, limit=limit)

            new_missing_count = df[columns_to_fill].isna().sum().sum()
            filled_count = old_missing_count - new_missing_count
            if filled_count > 0:
                ui.addInfoLogMessage(_("Filled {count} missing values").format(count=filled_count))  # cn: 已填充 {count} 个缺失值

            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to fill missing values: {error}").format(error=str(e)))  # cn: 填充缺失值失败：{error}
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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    dadata, _df, subset = select_datas_info
    
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Interpolate Values Parameter Settings"))  # cn: 插值填充参数设置
        .enum("method", label=_("Interpolation Method"),  # cn: 插值方法
              default="linear",
              options=[
                  option("linear", _("Linear interpolation")),  # cn: 线性插值
                  option("time", _("Time-based interpolation")),  # cn: 基于时间的插值
                  option("index", _("Use index values")),  # cn: 使用索引值
                  option("pad", _("Pad interpolation")),  # cn: 填充插值
                  option("nearest", _("Nearest interpolation")),  # cn: 最近邻插值
                  option("zero", _("Zero-order hold")),  # cn: 零阶保持
                  option("slinear", _("Spline linear interpolation")),  # cn: 样条线性插值
                  option("quadratic", _("Quadratic interpolation")),  # cn: 二次插值
                  option("cubic", _("Cubic interpolation")),  # cn: 三次插值
                  option("spline", _("Spline interpolation")),  # cn: 样条插值
                  option("barycentric", _("Barycentric interpolation")),  # cn: 重心插值
                  option("polynomial", _("Polynomial interpolation")),  # cn: 多项式插值
                  option("krogh", _("Krogh interpolation")),  # cn: Krogh插值
                  option("piecewise_polynomial", _("Piecewise polynomial interpolation")),  # cn: 分段多项式插值
                  option("pchip", _("PCHIP interpolation")),  # cn: PCHIP插值
                  option("akima", _("Akima interpolation")),  # cn: Akima插值
              ],
              description=_("Interpolation method to use"))  # cn: 使用的插值方法
        .int("limit", label=_("Interpolation Limit"),  # cn: 插值限制
             default=None, min=1, max=10000,
             description=_("Maximum number of consecutive NaN values to interpolate"))  # cn: 要插值的连续NaN值的最大数量
        .int("order", label=_("Spline Order"),  # cn: 样条阶数
             default=3, min=1, max=10,
             description=_("Order of the spline interpolator (used for 'spline' method)"))  # cn: 样条插值的阶数（用于'spline'方法）
        .bool("inplace", label=_("Inplace Interpolation"),  # cn: 原地插值
              default=True,
              description=_("Whether to interpolate in place or return a copy"))  # cn: 是否原地插值或返回副本
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.interpolate")
    if len(config) == 0:
        return None
    
    method = config.get("method", "linear")
    limit = config.get("limit", None)
    order = config.get("order", 3)
    inplace = config.get("inplace", True)
    
    def operation(original_df):
        columns_to_interpolate = subset if subset else original_df.columns.tolist()
        old_missing_count = original_df[columns_to_interpolate].isna().sum().sum()

        df = core_cleaning.interpolate_impl(original_df, subset=subset, method=method,
                                             limit=limit, order=order)

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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Replace Values"))
        .string("old_values", label=_("Values to Replace"),  # cn: 要替换的值
                default="",
                description=_("Values to find and replace (comma separated for multiple values)"))  # cn: 要查找和替换的值（多个值用逗号分隔）
        .string("new_value", label=_("Replacement Value"),  # cn: 替换值
                default="",
                description=_("Value to use as replacement"))  # cn: 用作替换的值
        .bool("case_sensitive", label=_("Case Sensitive"),  # cn: 区分大小写
              default=True,
              description=_("Consider letter case when matching values (for text only)"))  # cn: 匹配值时考虑字母大小写（仅适用于文本）
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.replace")
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

            columns_to_check = subset if subset else original_df.columns.tolist()
            old_count = original_df.copy()
            df = core_cleaning.replace_values_impl(original_df, subset=subset,
                                                    old_values=old_values, new_value=new_value,
                                                    case_sensitive=case_sensitive)

            replaced_count = 0
            for col in columns_to_check:
                replaced_count += (old_count[col] != df[col]).sum()

            if replaced_count > 0:
                ui.addInfoLogMessage(_("Replaced {count} values").format(count=replaced_count))  # cn: 已替换 {count} 个值
            else:
                ui.addInfoLogMessage(_("No matching values found to replace"))  # cn: 未找到可替换的匹配值

            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to replace values: {error}").format(error=str(e)))
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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Remove Outliers (IQR Method)"))  # cn: 移除异常值（IQR 方法）
        .group("detection", _("Detection Settings"))  # cn: 检测设置
        .float("multiplier", label=_("IQR Multiplier"),  # cn: IQR乘数
               default=1.5, min=0.5, max=10.0,
               description=_("Values beyond Q1-multiplier*IQR and Q3+multiplier*IQR are outliers (typical: 1.5-3)"))  # cn: 超出Q1-乘数*IQR和Q3+乘数*IQR的值被视为异常值（典型值：1.5-3）
        .end_group()
        .group("action", _("Action Settings"))  # cn: 操作设置
        .enum("action", label=_("Action on Outliers"),  # cn: 对异常值的操作
              default="remove",
              options=[
                  option("remove", _("Remove rows with outliers")),  # cn: 删除包含异常值的行
                  option("replace_mean", _("Replace outliers with column mean")),  # cn: 用列平均值替换异常值
                  option("replace_median", _("Replace outliers with column median")),  # cn: 用列中位数替换异常值
                  option("replace_boundary", _("Replace outliers with boundary values")),  # cn: 用边界值替换异常值
                  option("replace_custom", _("Replace outliers with custom value")),  # cn: 用自定义值替换异常值
              ],
              description=_("How to handle detected outliers"))  # cn: 如何处理检测到的异常值
        .float("custom_value", label=_("Custom Replacement Value"),  # cn: 自定义替换值
               default=0.0,
               description=_("Value to use for replacement when action is 'replace_custom'"))  # cn: 当操作为'replace_custom'时使用的替换值
        .bool("reindex", label=_("Reset Row Numbers"),  # cn: 重置行号
              default=True,
              description=_("Renumber rows after removing outliers (only for 'remove' action)"))  # cn: 删除异常值后重新编号行（仅适用于'remove'操作）
        .end_group()
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.remove_outliers_iqr")
    if len(config) == 0:
        return None
    
    multiplier = config.get("multiplier", 1.5)
    action = config.get("action", "remove")
    custom_value = config.get("custom_value", 0.0)
    reindex = config.get("reindex", True)
    
    def operation(original_df):
        try:
            old_len = len(original_df)
            df = core_cleaning.remove_outliers_iqr_impl(original_df, columns=subset,
                                                         multiplier=multiplier, action=action,
                                                         custom_value=custom_value, reindex=reindex)
            if action == "remove":
                removed_rows = old_len - len(df)
                if removed_rows > 0:
                    ui.addInfoLogMessage(_("Removed {count} rows containing outliers using IQR method").format(count=removed_rows))  # cn: 使用 IQR 方法移除了 {count} 行含异常值的数据
                else:
                    ui.addInfoLogMessage(_("No outliers detected using IQR method"))  # cn: 使用 IQR 方法未检测到异常值
            else:
                ui.addInfoLogMessage(_("Outlier values processed using IQR method ({action})").format(action=action))  # cn: 使用 IQR 方法处理了异常值（{action}）
            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to process outliers: {error}").format(error=str(e)))
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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Remove Outliers (Z-score)"))  # cn: 移除异常值（Z-score 方法）
        .group("detection", _("Detection Settings"))  # cn: 检测设置
        .float("threshold", label=_("Z-score Threshold"),  # cn: Z-score阈值
               default=3.0, min=1.0, max=10.0,
               description=_("Values with absolute Z-score above this threshold are outliers (typical: 3)"))  # cn: 绝对值超过此阈值的Z-score被视为异常值（典型值：3）
        .bool("robust", label=_("Use Robust Statistics"),  # cn: 使用稳健统计
              default=False,
              description=_("Use median and MAD instead of mean and std (better for non-normal data)"))  # cn: 使用中位数和MAD代替均值和标准差（适用于非正态数据）
        .end_group()
        .group("action", _("Action Settings"))  # cn: 操作设置
        .enum("action", label=_("Action on Outliers"),  # cn: 对异常值的操作
              default="remove",
              options=[
                  option("remove", _("Remove rows with outliers")),  # cn: 删除包含异常值的行
                  option("replace_mean", _("Replace outliers with column mean")),  # cn: 用列平均值替换异常值
                  option("replace_median", _("Replace outliers with column median")),  # cn: 用列中位数替换异常值
                  option("replace_boundary", _("Replace outliers with boundary values")),  # cn: 用边界值替换异常值
                  option("replace_custom", _("Replace outliers with custom value")),  # cn: 用自定义值替换异常值
              ],
              description=_("How to handle detected outliers"))  # cn: 如何处理检测到的异常值
        .float("custom_value", label=_("Custom Replacement Value"),  # cn: 自定义替换值
               default=0.0,
               description=_("Value to use for replacement when action is 'replace_custom'"))  # cn: 当操作为'replace_custom'时使用的替换值
        .bool("reindex", label=_("Reset Row Numbers"),  # cn: 重置行号
              default=True,
              description=_("Renumber rows after removing outliers (only for 'remove' action)"))  # cn: 删除异常值后重新编号行（仅适用于'remove'操作）
        .end_group()
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.remove_outliers_zscore")
    if len(config) == 0:
        return None
    
    threshold = config.get("threshold", 3.0)
    robust = config.get("robust", False)
    action = config.get("action", "remove")
    custom_value = config.get("custom_value", 0.0)
    reindex = config.get("reindex", True)
    
    def operation(original_df):
        try:
            old_len = len(original_df)
            df = core_cleaning.remove_outliers_zscore_impl(original_df, columns=subset,
                                                            threshold=threshold, robust=robust,
                                                            action=action, custom_value=custom_value,
                                                            reindex=reindex)
            if action == "remove":
                removed_rows = old_len - len(df)
                if removed_rows > 0:
                    ui.addInfoLogMessage(_("Removed {count} rows containing outliers using Z-score method").format(count=removed_rows))  # cn: 使用 Z-score 方法移除了 {count} 行含异常值的数据
                else:
                    ui.addInfoLogMessage(_("No outliers detected using Z-score method"))  # cn: 使用 Z-score 方法未检测到异常值
            else:
                ui.addInfoLogMessage(_("Outlier values processed using Z-score method ({action})").format(action=action))  # cn: 使用 Z-score 方法处理了异常值（{action}）
            return df
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to process outliers: {error}").format(error=str(e)))
            raise

    return _execute_dataframe_operation(
        dadata,
        _("Remove Outliers (Z-Score)"),  # cn: 移除异常值（Z-Score）
        operation,
        _("Removed {removed_rows} rows outliers")  # cn: 处理了{removed_rows}行异常值
    )

def threshold_filter() -> Optional[int]:
    """
    Filter rows based on value thresholds (remove too high, too low, or in/out of range)
    基于数值上下限过滤行（删除过高、过低、在范围内或范围外的值）
    """
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    # 获取数值列
    numeric_cols = _df.select_dtypes(include=[np.number]).columns.tolist()
    if not numeric_cols:
        ui.addWarningLogMessage(_("No numeric columns found in selected data. Please select numeric columns for threshold filtering."))  # cn: 所选数据中未找到数值列。请选择数值列进行上下限过滤。
        return None
    
    # 限制只选择数值列
    subset = [col for col in subset if col in numeric_cols]
    if not subset:
        ui.addWarningLogMessage(_("No numeric columns selected. Please select at least one numeric column."))  # cn: 未选择数值列。请至少选择一个数值列。
        return None
    
    cfg = (
        FormBuilder(_("Threshold Filter"))  # cn: 上下限过滤设置
        .group("filter_conditions", _("Filter Conditions"))  # cn: 过滤条件
        .enum("filter_type", label=_("Filter Type"),  # cn: 过滤类型
              default="greater_than",
              options=[
                  option("greater_than", _("Remove values GREATER THAN threshold (too high)")),  # cn: 删除大于阈值的值（过高）
                  option("less_than", _("Remove values LESS THAN threshold (too low)")),  # cn: 删除小于阈值的值（过低）
                  option("in_range", _("Remove values WITHIN range (keep outliers)")),  # cn: 删除范围内的值（保留异常值）
                  option("out_of_range", _("Remove values OUTSIDE range (keep normal range)")),  # cn: 删除范围外的值（保留正常范围）
              ],
              description=_("Select the type of filtering you want to apply: remove values that are too high, too low, within a specific range, or outside a specific range."))  # cn: 选择要应用的过滤类型：删除过高的值、过低的值、特定范围内的值或特定范围外的值。
        .float("lower_threshold", label=_("Lower Threshold"),  # cn: 下限阈值
               default=0.0,
               description=_("The minimum acceptable value. Rows with values below this (depending on filter type) will be removed."))  # cn: 可接受的最小值。低于此值的行（根据过滤类型）将被删除。
        .float("upper_threshold", label=_("Upper Threshold"),  # cn: 上限阈值
               default=100.0,
               description=_("The maximum acceptable value. Rows with values above this (depending on filter type) will be removed."))  # cn: 可接受的最大值。高于此值的行（根据过滤类型）将被删除。
        .enum("row_removal_logic", label=_("Row Removal Logic"),  # cn: 行删除逻辑
              default="any",
              options=[
                  option("any", _("Remove row if ANY selected column meets the condition")),  # cn: 任意选中的列满足条件时删除行
                  option("all", _("Remove row only if ALL selected columns meet the condition")),  # cn: 所有选中的列都满足条件时才删除行
              ],
              description=_("When multiple columns are selected: 'Any' removes the row if any column violates the threshold; 'All' only removes the row if all columns violate the threshold."))  # cn: 当选择多个列时：'任意'表示任意列违反阈值就删除行；'所有'表示所有列都违反阈值才删除行。
        .end_group()
        .group("advanced", _("Advanced Settings"))  # cn: 高级设置
        .bool("treat_nan_as_violation", label=_("Treat NaN as Threshold Violation"),  # cn: 将NaN视为违反阈值
              default=False,
              description=_("If enabled, rows with NaN (missing) values in selected columns will also be removed. If disabled, NaN values will be ignored."))  # cn: 如果启用，选中的列中包含NaN（缺失）值的行也将被删除。如果禁用，NaN值将被忽略。
        .bool("reindex", label=_("Reset Row Numbers"),  # cn: 重置行号
              default=True,
              description=_("Renumber rows from 0 after filtering"))  # cn: 过滤后从0开始重新编号行
        .end_group()
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.threshold_filter")
    if len(config) == 0:
        return None
    
    filter_type = config.get("filter_type", "greater_than")
    lower_threshold = config.get("lower_threshold", 0.0)
    upper_threshold = config.get("upper_threshold", 100.0)
    row_removal_logic = config.get("row_removal_logic", "any")
    treat_nan_as_violation = config.get("treat_nan_as_violation", False)
    reindex = config.get("reindex", True)
    
    def operation(df):
        try:
            return core_cleaning.threshold_filter_impl(df, subset=subset, filter_type=filter_type,
                                                        lower=lower_threshold, upper=upper_threshold,
                                                        row_logic=row_removal_logic,
                                                        treat_nan=treat_nan_as_violation, reindex=reindex)
        except Exception as e:
            ui.addCriticalLogMessage(_("Threshold filtering failed: {error}").format(error=str(e)))  # cn: 阈值过滤失败：{error}
            raise
    
    # 根据过滤类型生成成功消息
    if filter_type == "greater_than":
        success_msg = _("Removed {removed_rows} rows with values greater than {upper_threshold}")  # cn: 删除了{removed_rows}个值大于{upper_threshold}的行
        success_msg = success_msg.format(upper_threshold=upper_threshold)
    elif filter_type == "less_than":
        success_msg = _("Removed {removed_rows} rows with values less than {lower_threshold}")  # cn: 删除了{removed_rows}个值小于{lower_threshold}的行
        success_msg = success_msg.format(lower_threshold=lower_threshold)
    elif filter_type == "in_range":
        success_msg = _("Removed {removed_rows} rows with values between {lower_threshold} and {upper_threshold}")  # cn: 删除了{removed_rows}个值在{lower_threshold}和{upper_threshold}之间的行
        success_msg = success_msg.format(lower_threshold=lower_threshold, upper_threshold=upper_threshold)
    elif filter_type == "out_of_range":
        success_msg = _("Removed {removed_rows} rows with values outside {lower_threshold} to {upper_threshold} range")  # cn: 删除了{removed_rows}个值在{lower_threshold}到{upper_threshold}范围之外的行
        success_msg = success_msg.format(lower_threshold=lower_threshold, upper_threshold=upper_threshold)
    else:
        success_msg = _("Filtered {removed_rows} rows based on threshold conditions")  # cn: 根据阈值条件过滤了{removed_rows}行
    
    return _execute_dataframe_operation(
        dadata,
        _("Threshold Filter"),  # cn: 上下限过滤
        operation,
        success_msg
    )

# ============================================================================
# 数据转换（简化设置）
# ============================================================================

def transform_skewed_data() -> Optional[int]:
    """
    Transform skewed numerical data to improve distribution
    转换偏态数值数据以改善分布
    """
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Transform Skewed Data"))
        .enum("method", label=_("Transformation Method"),  # cn: 转换方法
              default="log",
              options=[
                  option("log", _("Logarithm transformation (log(x))")),  # cn: 对数转换（log(x)）
                  option("sqrt", _("Square root transformation (sqrt(x))")),  # cn: 平方根转换（sqrt(x)）
                  option("reciprocal", _("Reciprocal transformation (1/x)")),  # cn: 倒数转换（1/x）
                  option("power", _("Power transformation (x^lambda)")),  # cn: 幂转换（x^lambda）
              ],
              description=_("Choose transformation method for reducing skewness"))  # cn: 选择减少偏度的转换方法
        .float("lambda_value", label=_("Power Lambda"),  # cn: 幂Lambda值
               default=0.5, min=-5.0, max=5.0,
               description=_("Power value for power transformation (ignored for other methods)"))  # cn: 幂转换的指数值（其他方法忽略此值）
        .bool("add_one", label=_("Add 1 before transformation"),  # cn: 转换前加1
              default=True,
              description=_("Add 1 to values before log/sqrt transformation (handles zeros)"))  # cn: 在对数/平方根转换前加1（处理零值）
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.transform_skewed")
    if len(config) == 0:
        return None
    
    method = config.get("method", "log")
    lambda_value = config.get("lambda_value", 0.5)
    add_one = config.get("add_one", True)
    
    def operation(original_df):
        try:
            return core_cleaning.transform_skewed_impl(original_df, columns=subset,
                                                        method=method, lambda_value=lambda_value,
                                                        add_one=add_one)
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to transform data: {error}").format(error=str(e)))
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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    dadata, _df, subset = select_datas_info
    
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Scale Data Parameter Settings"))  # cn: 数据缩放参数设置
        .enum("scaling_method", label=_("Scaling Method"),  # cn: 缩放方法
              default="standard",
              options=[
                  option("standard", _("Standard scaling (z-score: (x-mean)/std)")),  # cn: 标准缩放（z-score：(x-均值)/标准差）
                  option("minmax", _("Min-Max scaling (to [0, 1] range)")),  # cn: 最小-最大缩放（到[0, 1]范围）
                  option("robust", _("Robust scaling (using median and IQR)")),  # cn: 稳健缩放（使用中位数和IQR）
                  option("maxabs", _("MaxAbs scaling (to [-1, 1] range)")),  # cn: 最大绝对值缩放（到[-1, 1]范围）
                  option("unit_vector", _("Unit vector scaling (norm=1)")),  # cn: 单位向量缩放（范数=1）
              ],
              description=_("Method for scaling data"))  # cn: 数据缩放方法
        .float("feature_range_min", label=_("Feature Range (Min)"),  # cn: 特征范围（最小值）
               default=0.0,
               description=_("Minimum value for min-max scaling range"))  # cn: 最小-最大缩放范围的最小值
        .float("feature_range_max", label=_("Feature Range (Max)"),  # cn: 特征范围（最大值）
               default=1.0,
               description=_("Maximum value for min-max scaling range"))  # cn: 最小-最大缩放范围的最大值
        .bool("with_mean", label=_("With Mean (for standard scaling)"),  # cn: 使用均值（用于标准缩放）
              default=True,
              description=_("Whether to center the data before scaling (for standard scaling)"))  # cn: 是否在缩放前将数据中心化（用于标准缩放）
        .bool("with_std", label=_("With Std (for standard scaling)"),  # cn: 使用标准差（用于标准缩放）
              default=True,
              description=_("Whether to scale to unit variance (for standard scaling)"))  # cn: 是否缩放到单位方差（用于标准缩放）
        .bool("create_new_columns", label=_("Create New Columns"),  # cn: 创建新列
              default=True,
              description=_("Create new columns instead of replacing original data"))  # cn: 创建新列而不是替换原始数据
        .string("suffix", label=_("Column Suffix"),  # cn: 列后缀
                default="_scaled",
                description=_("Suffix for new column names"))  # cn: 新列名的后缀
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.scale_data")
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
        try:
            return core_cleaning.scale_data_impl(original_df, columns=subset,
                                                  method=scaling_method,
                                                  feature_range=(feature_range_min, feature_range_max),
                                                  with_mean=with_mean, with_std=with_std,
                                                  create_new=create_new_columns, suffix=suffix)
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to scale data: {error}").format(error=str(e)))
            raise

    return _execute_dataframe_operation(
        dadata,
        _("Scale Data"),  # cn: 数据缩放
        operation,
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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Clean Text Strings"))
        .enum("trim_type", label=_("Trim Spaces"),  # cn: 修剪空格
              default="both",
              options=[
                  option("both", _("Remove spaces from both sides")),  # cn: 删除两侧空格
                  option("left", _("Remove spaces from left side only")),  # cn: 仅删除左侧空格
                  option("right", _("Remove spaces from right side only")),  # cn: 仅删除右侧空格
                  option("none", _("Do not trim spaces")),  # cn: 不修剪空格
              ],
              description=_("Remove leading/trailing spaces from text"))  # cn: 从文本中删除前导/尾随空格
        .bool("lowercase", label=_("Convert to Lowercase"),  # cn: 转换为小写
              default=False,
              description=_("Convert all text to lowercase letters"))  # cn: 将所有文本转换为小写字母
        .bool("remove_extra_spaces", label=_("Remove Extra Spaces"),  # cn: 删除额外空格
              default=True,
              description=_("Replace multiple spaces with single space"))  # cn: 将多个空格替换为单个空格
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.clean_text")
    if len(config) == 0:
        return None
    
    trim_type = config.get("trim_type", "both")
    lowercase = config.get("lowercase", False)
    remove_extra_spaces = config.get("remove_extra_spaces", True)
    
    def operation(original_df):
        try:
            return core_cleaning.clean_text_impl(original_df, columns=subset,
                                                  trim_type=trim_type, lowercase=lowercase,
                                                  remove_extra_spaces=remove_extra_spaces)
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to clean text: {error}").format(error=str(e)))  # cn: 文本清洗失败：{error}
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
    select_datas_info = utils.get_select_dataframe_and_subset_index()
    if not select_datas_info:
        return None
    
    dadata, _df, subset = select_datas_info
    ui = da_app.getCore().getUiInterface()
    
    cfg = (
        FormBuilder(_("Label Encoding"))
        .bool("keep_original", label=_("Keep Original Column"),  # cn: 保留原始列
              default=True,
              description=_("Keep original text column and create new encoded column"))  # cn: 保留原始文本列并创建新的编码列
        .build()
    )

    config = ui.getConfigValues(cfg, "dataframecleaner.encode_label")
    if len(config) == 0:
        return None
    
    keep_original = config.get("keep_original", True)
    
    def operation(original_df):
        try:
            return core_cleaning.encode_label_impl(original_df, columns=subset,
                                                    keep_original=keep_original)
        except Exception as e:
            ui.addCriticalLogMessage(_("Failed to encode labels: {error}").format(error=str(e)))  # cn: 标签编码失败：{error}
            raise
    
    return _execute_dataframe_operation(
        dadata,
        _("Label Encoding"),  # cn: 标签编码
        operation,
        ""
    )
