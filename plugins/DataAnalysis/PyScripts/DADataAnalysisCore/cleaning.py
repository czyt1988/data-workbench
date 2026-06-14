# -*- coding: utf-8 -*-
"""
数据清洗核心算法模块

提供纯 pandas/numpy 实现的数据清洗函数。
所有函数接收 DataFrame 和参数，返回新的 DataFrame，不依赖 GUI 或 DA 应用框架。
"""

from typing import List, Optional, Any, Union
import pandas as pd
import numpy as np


def dropna_impl(df: pd.DataFrame, subset: Optional[List[str]] = None,
                how: str = "any", min_non_na: int = 0,
                reindex: bool = True) -> pd.DataFrame:
    """
    删除包含缺失值的行

    :param df: 输入 DataFrame
    :param subset: 检查缺失值的列名列表，None 表示所有列
    :param how: "any" 或 "all"
    :param min_non_na: 保留行所需的最少非缺失值数量，0 表示禁用
    :param reindex: 是否重置索引
    :return: 处理后的 DataFrame
    """
    if min_non_na > 0:
        result = df.dropna(axis=0, subset=subset, thresh=min_non_na)
    else:
        result = df.dropna(axis=0, subset=subset, how=how)
    if reindex:
        result = result.reset_index(drop=True)
    return result


def drop_duplicates_impl(df: pd.DataFrame, subset: Optional[List[str]] = None,
                         keep: str = "first",
                         ignore_index: bool = True) -> pd.DataFrame:
    """
    删除重复行

    :param df: 输入 DataFrame
    :param subset: 判断重复的列名列表
    :param keep: "first"/"last"/"none"
    :param ignore_index: 是否重置索引
    :return: 处理后的 DataFrame
    """
    keep_val = False if keep == "none" else keep
    return df.drop_duplicates(subset=subset, keep=keep_val, ignore_index=ignore_index)


def fillna_impl(df: pd.DataFrame, subset: Optional[List[str]] = None,
                method: str = "value", value: Any = 0.0,
                limit: Optional[int] = None) -> pd.DataFrame:
    """
    填充缺失值

    :param df: 输入 DataFrame
    :param subset: 要填充的列名列表，None 表示所有列
    :param method: 填充方法（value/forward/backward/mean/median/mode）
    :param value: 自定义填充值（method="value" 时有效）
    :param limit: 最大连续填充数
    :return: 处理后的 DataFrame
    """
    result = df.copy()
    columns_to_fill = subset if subset else result.columns.tolist()

    if method == "value":
        result[columns_to_fill] = result[columns_to_fill].fillna(value, limit=limit)
    elif method == "forward":
        result[columns_to_fill] = result[columns_to_fill].ffill(limit=limit)
    elif method == "backward":
        result[columns_to_fill] = result[columns_to_fill].bfill(limit=limit)
    elif method == "mean":
        for col in columns_to_fill:
            if pd.api.types.is_numeric_dtype(result[col]):
                result[col] = result[col].fillna(result[col].mean(), limit=limit)
    elif method == "median":
        for col in columns_to_fill:
            if pd.api.types.is_numeric_dtype(result[col]):
                result[col] = result[col].fillna(result[col].median(), limit=limit)
    elif method == "mode":
        for col in columns_to_fill:
            if not result[col].mode().empty:
                result[col] = result[col].fillna(result[col].mode().iloc[0], limit=limit)
    return result


def interpolate_impl(df: pd.DataFrame, subset: Optional[List[str]] = None,
                     method: str = "linear", limit: Optional[int] = None,
                     order: int = 3) -> pd.DataFrame:
    """
    插值填充缺失值

    :param df: 输入 DataFrame
    :param subset: 要插值的列名列表
    :param method: 插值方法
    :param limit: 最大连续插值数
    :param order: 样条阶数（method="spline" 时有效）
    :return: 处理后的 DataFrame
    """
    result = df.copy()
    columns_to_interpolate = subset if subset else result.columns.tolist()

    for col in columns_to_interpolate:
        if pd.api.types.is_numeric_dtype(result[col]):
            try:
                if method == "spline":
                    result[col] = result[col].interpolate(method=method, order=order, limit=limit, inplace=False)
                elif method == "polynomial":
                    result[col] = result[col].interpolate(method=method, order=order, limit=limit, inplace=False)
                else:
                    result[col] = result[col].interpolate(method=method, limit=limit, inplace=False)
            except Exception:
                pass
    return result


def replace_values_impl(df: pd.DataFrame, subset: Optional[List[str]] = None,
                        old_values: Optional[List[str]] = None,
                        new_value: Any = "",
                        case_sensitive: bool = True) -> pd.DataFrame:
    """
    替换特定值

    :param df: 输入 DataFrame
    :param subset: 要检查的列名列表
    :param old_values: 要替换的旧值列表
    :param new_value: 替换值
    :param case_sensitive: 是否区分大小写
    :return: 处理后的 DataFrame
    """
    if not old_values:
        return df.copy()

    result = df.copy()
    columns_to_check = subset if subset else result.columns.tolist()

    for col in columns_to_check:
        for old_val in old_values:
            try:
                if pd.api.types.is_string_dtype(result[col]):
                    if case_sensitive:
                        mask = result[col] == old_val
                    else:
                        mask = result[col].astype(str).str.lower() == old_val.lower()
                else:
                    try:
                        old_val_num = float(old_val)
                        mask = result[col] == old_val_num
                    except (ValueError, TypeError):
                        mask = result[col] == old_val

                if mask.sum() > 0:
                    result.loc[mask, col] = new_value
            except Exception:
                pass
    return result


def remove_outliers_iqr_impl(df: pd.DataFrame, columns: Optional[List[str]] = None,
                             multiplier: float = 1.5, action: str = "remove",
                             custom_value: float = 0.0,
                             reindex: bool = True) -> pd.DataFrame:
    """
    使用 IQR 方法处理异常值

    :param df: 输入 DataFrame
    :param columns: 要检查的列名列表，None 表示所有数值列
    :param multiplier: IQR 乘数
    :param action: 处理方式（remove/replace_mean/replace_median/replace_boundary/replace_custom）
    :param custom_value: 自定义替换值
    :param reindex: 删除后是否重置索引
    :return: 处理后的 DataFrame
    """
    if not columns:
        columns = df.select_dtypes(include=[np.number]).columns.tolist()
    else:
        columns = [c for c in columns if pd.api.types.is_numeric_dtype(df[c])]

    if not columns:
        return df.copy()

    result = df.copy()

    if action == "remove":
        mask = pd.Series([True] * len(result), index=result.index)
        for col in columns:
            Q1 = result[col].quantile(0.25)
            Q3 = result[col].quantile(0.75)
            IQR = Q3 - Q1
            lower_bound = Q1 - multiplier * IQR
            upper_bound = Q3 + multiplier * IQR
            outliers_mask = (result[col] < lower_bound) | (result[col] > upper_bound)
            mask = mask & ~outliers_mask
        result = result[mask]
        if reindex:
            result = result.reset_index(drop=True)
    else:
        for col in columns:
            Q1 = result[col].quantile(0.25)
            Q3 = result[col].quantile(0.75)
            IQR = Q3 - Q1
            lower_bound = Q1 - multiplier * IQR
            upper_bound = Q3 + multiplier * IQR
            outliers_mask = (result[col] < lower_bound) | (result[col] > upper_bound)

            if not outliers_mask.any():
                continue

            if action == "replace_mean":
                replacement = result[col].mean()
            elif action == "replace_median":
                replacement = result[col].median()
            elif action == "replace_boundary":
                result.loc[outliers_mask & (result[col] > upper_bound), col] = upper_bound
                result.loc[outliers_mask & (result[col] < lower_bound), col] = lower_bound
                continue
            elif action == "replace_custom":
                replacement = custom_value
            else:
                continue

            result.loc[outliers_mask, col] = replacement

    return result


def remove_outliers_zscore_impl(df: pd.DataFrame, columns: Optional[List[str]] = None,
                                threshold: float = 3.0, robust: bool = False,
                                action: str = "remove", custom_value: float = 0.0,
                                reindex: bool = True) -> pd.DataFrame:
    """
    使用 Z-score 方法处理异常值

    :param df: 输入 DataFrame
    :param columns: 要检查的列名列表，None 表示所有数值列
    :param threshold: Z-score 阈值
    :param robust: 是否使用稳健统计（中位数和 MAD）
    :param action: 处理方式（remove/replace_mean/replace_median/replace_boundary/replace_custom）
    :param custom_value: 自定义替换值
    :param reindex: 删除后是否重置索引
    :return: 处理后的 DataFrame
    """
    if not columns:
        columns = df.select_dtypes(include=[np.number]).columns.tolist()
    else:
        columns = [c for c in columns if pd.api.types.is_numeric_dtype(df[c])]

    if not columns:
        return df.copy()

    result = df.copy()

    if action == "remove":
        mask = pd.Series([True] * len(result), index=result.index)
        for col in columns:
            if robust:
                median = result[col].median()
                mad = np.median(np.abs(result[col] - median))
                std_estimate = mad * 1.4826 if mad != 0 else 1e-10
                z_scores = (result[col] - median) / std_estimate
            else:
                mean = result[col].mean()
                std = result[col].std()
                std = std if std != 0 else 1e-10
                z_scores = (result[col] - mean) / std
            outliers_mask = np.abs(z_scores) > threshold
            mask = mask & ~outliers_mask
        result = result[mask]
        if reindex:
            result = result.reset_index(drop=True)
    else:
        for col in columns:
            if robust:
                median = result[col].median()
                mad = np.median(np.abs(result[col] - median))
                std_estimate = mad * 1.4826 if mad != 0 else 1e-10
                z_scores = (result[col] - median) / std_estimate
            else:
                mean = result[col].mean()
                std = result[col].std()
                std = std if std != 0 else 1e-10
                z_scores = (result[col] - mean) / std

            outliers_mask = np.abs(z_scores) > threshold
            if not outliers_mask.any():
                continue

            if action == "replace_mean":
                replacement = result[col].mean()
            elif action == "replace_median":
                replacement = result[col].median()
            elif action == "replace_boundary":
                if robust:
                    upper_bound = median + threshold * std_estimate
                    lower_bound = median - threshold * std_estimate
                    center = median
                else:
                    upper_bound = mean + threshold * std
                    lower_bound = mean - threshold * std
                    center = mean
                result.loc[outliers_mask & (result[col] > center), col] = upper_bound
                result.loc[outliers_mask & (result[col] < center), col] = lower_bound
                continue
            elif action == "replace_custom":
                replacement = custom_value
            else:
                continue

            result.loc[outliers_mask, col] = replacement

    return result


def threshold_filter_impl(df: pd.DataFrame, subset: Optional[List[str]] = None,
                          filter_type: str = "greater_than",
                          lower: float = 0.0, upper: float = 100.0,
                          row_logic: str = "any",
                          treat_nan: bool = False,
                          reindex: bool = True) -> pd.DataFrame:
    """
    基于阈值过滤行

    :param df: 输入 DataFrame
    :param subset: 要检查的列名列表
    :param filter_type: 过滤类型（greater_than/less_than/in_range/out_of_range）
    :param lower: 下限阈值
    :param upper: 上限阈值
    :param row_logic: 行删除逻辑（any/all）
    :param treat_nan: 是否将 NaN 视为违反阈值
    :param reindex: 是否重置索引
    :return: 过滤后的 DataFrame
    """
    mask = pd.Series([False] * len(df), index=df.index)

    for col in subset:
        if col not in df.columns:
            continue
        col_data = df[col]

        if filter_type == "greater_than":
            col_mask = col_data > upper
        elif filter_type == "less_than":
            col_mask = col_data < lower
        elif filter_type == "in_range":
            col_mask = (col_data >= lower) & (col_data <= upper)
        elif filter_type == "out_of_range":
            col_mask = (col_data < lower) | (col_data > upper)
        else:
            col_mask = pd.Series([False] * len(df), index=df.index)

        col_mask = col_mask.fillna(True if treat_nan else False)

        if row_logic == "any":
            mask = mask | col_mask
        else:
            if mask.sum() == 0 and not mask.any():
                mask = col_mask
            else:
                mask = mask & col_mask

    result = df[~mask]
    if reindex:
        result = result.reset_index(drop=True)
    return result


def transform_skewed_impl(df: pd.DataFrame, columns: Optional[List[str]] = None,
                          method: str = "log", lambda_value: float = 0.5,
                          add_one: bool = True) -> pd.DataFrame:
    """
    偏态数据转换

    :param df: 输入 DataFrame
    :param columns: 要转换的列名列表，None 表示所有数值列
    :param method: 转换方法（log/sqrt/reciprocal/power/boxcox）
    :param lambda_value: 幂转换的 lambda 值
    :param add_one: 转换前是否加 1（处理零值）
    :return: 转换后的 DataFrame
    """
    if not columns:
        columns = df.select_dtypes(include=[np.number]).columns.tolist()
    else:
        columns = [c for c in columns if pd.api.types.is_numeric_dtype(df[c])]

    if not columns:
        return df.copy()

    result = df.copy()
    for col in columns:
        col_data = result[col].copy()
        if method == "log":
            if add_one:
                result[col] = np.log1p(col_data.where(col_data >= 0, np.nan))
            else:
                result[col] = np.log(col_data.where(col_data > 0, np.nan))
        elif method == "sqrt":
            if add_one:
                result[col] = np.sqrt(col_data.where(col_data >= -1, np.nan) + 1)
            else:
                result[col] = np.sqrt(col_data.where(col_data >= 0, np.nan))
        elif method == "reciprocal":
            result[col] = 1 / col_data.where(col_data != 0, np.nan)
        elif method == "power":
            result[col] = col_data ** lambda_value
        elif method == "boxcox":
            try:
                from scipy import stats
                data = col_data.dropna()
                if add_one:
                    data = data + 1
                if (data <= 0).any():
                    continue
                transformed, _ = stats.boxcox(data)
                result.loc[data.index, col] = transformed
            except (ImportError, ValueError):
                pass
    return result


def scale_data_impl(df: pd.DataFrame, columns: Optional[List[str]] = None,
                    method: str = "standard",
                    feature_range: tuple = (0.0, 1.0),
                    with_mean: bool = True, with_std: bool = True,
                    create_new: bool = True, suffix: str = "_scaled") -> pd.DataFrame:
    """
    数据缩放（标准化/归一化）

    :param df: 输入 DataFrame
    :param columns: 要缩放的列名列表，None 表示所有数值列
    :param method: 缩放方法（standard/minmax/robust/maxabs/unit_vector）
    :param feature_range: minmax 缩放的目标范围
    :param with_mean: 标准缩放时是否中心化
    :param with_std: 标准缩放时是否缩放到单位方差
    :param create_new: 是否创建新列（True）还是替换原列
    :param suffix: 新列名后缀
    :return: 缩放后的 DataFrame
    """
    if not columns:
        columns = df.select_dtypes(include=[np.number]).columns.tolist()
    else:
        columns = [c for c in columns if pd.api.types.is_numeric_dtype(df[c])]

    if not columns:
        return df.copy()

    result = df.copy()
    range_min, range_max = feature_range

    for col in columns:
        col_data = result[col].copy()

        if method == "standard":
            mean = col_data.mean() if with_mean else 0
            std = col_data.std() if with_std else 1
            std = std if std != 0 else 1e-10
            scaled = (col_data - mean) / std
        elif method == "minmax":
            col_min = col_data.min()
            col_max = col_data.max()
            if col_max != col_min:
                scaled = (col_data - col_min) / (col_max - col_min)
                scaled = scaled * (range_max - range_min) + range_min
            else:
                scaled = pd.Series([range_min] * len(col_data), dtype=float, index=col_data.index)
        elif method == "robust":
            median = col_data.median()
            q1 = col_data.quantile(0.25)
            q3 = col_data.quantile(0.75)
            iqr = q3 - q1
            iqr = iqr if iqr != 0 else 1e-10
            scaled = (col_data - median) / iqr
        elif method == "maxabs":
            max_abs = np.abs(col_data).max()
            max_abs = max_abs if max_abs != 0 else 1e-10
            scaled = col_data / max_abs
        elif method == "unit_vector":
            norm = np.sqrt((col_data ** 2).sum())
            norm = norm if norm != 0 else 1e-10
            scaled = col_data / norm
        else:
            continue

        if create_new:
            result[f"{col}{suffix}"] = scaled
        else:
            result[col] = scaled

    return result


def clean_text_impl(df: pd.DataFrame, columns: Optional[List[str]] = None,
                    trim_type: str = "both", lowercase: bool = False,
                    remove_extra_spaces: bool = True) -> pd.DataFrame:
    """
    清理和标准化文本字符串

    :param df: 输入 DataFrame
    :param columns: 要清理的列名列表，None 表示所有文本列
    :param trim_type: 修剪空格方式（both/left/right/none）
    :param lowercase: 是否转换为小写
    :param remove_extra_spaces: 是否将多个空格替换为单个空格
    :return: 处理后的 DataFrame
    """
    if columns:
        text_cols = [c for c in columns if df[c].dtype == 'object']
    else:
        text_cols = df.select_dtypes(include=['object']).columns.tolist()

    if not text_cols:
        return df.copy()

    result = df.copy()
    for col in text_cols:
        if trim_type == "both":
            result[col] = result[col].str.strip()
        elif trim_type == "left":
            result[col] = result[col].str.lstrip()
        elif trim_type == "right":
            result[col] = result[col].str.rstrip()

        if remove_extra_spaces:
            result[col] = result[col].str.replace(r'\s+', ' ', regex=True)

        if lowercase:
            result[col] = result[col].str.lower()
    return result


def encode_label_impl(df: pd.DataFrame, columns: Optional[List[str]] = None,
                      keep_original: bool = True) -> pd.DataFrame:
    """
    标签编码：将分类文本转换为数字标签

    :param df: 输入 DataFrame
    :param columns: 要编码的列名列表，None 表示所有文本列
    :param keep_original: 是否保留原始列
    :return: 编码后的 DataFrame
    """
    if columns:
        text_cols = [c for c in columns if df[c].dtype == 'object']
    else:
        text_cols = df.select_dtypes(include=['object']).columns.tolist()

    if not text_cols:
        return df.copy()

    result = df.copy()
    for col in text_cols:
        unique_values = result[col].unique()
        value_mapping = {val: i for i, val in enumerate(unique_values)}
        if keep_original:
            result[f"{col}_encoded"] = result[col].map(value_mapping)
        else:
            result[col] = result[col].map(value_mapping)
    return result
