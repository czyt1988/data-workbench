# -*- coding: utf-8 -*-
"""
数据操作模块

提供 DataFrame 数据操作函数，供 GUI Worker 和工作流节点调用。
所有函数均为纯 Python 实现，不依赖 GUI，返回新的 DataFrame（不修改输入）。
"""

import pandas as pd


def sort_dataframe(df: pd.DataFrame, columns, ascending: bool = True) -> pd.DataFrame:
    """
    按指定列对 DataFrame 进行排序

    :param df: 输入 DataFrame
    :param columns: 排序列名（字符串或字符串列表）
    :param ascending: 是否升序，默认 True
    :return: 排序后的新 DataFrame
    """
    if isinstance(columns, str):
        columns = [columns]
    return df.sort_values(by=columns, ascending=ascending).copy()


def query_dataframe(df: pd.DataFrame, query_str: str) -> pd.DataFrame:
    """
    使用 pandas query 表达式筛选 DataFrame

    :param df: 输入 DataFrame
    :param query_str: query 表达式（如 "age > 25 and name == 'John'"）
    :return: 筛选后的新 DataFrame
    """
    return df.query(query_str).copy()


def search_dataframe(df: pd.DataFrame, column: str, pattern: str, case_sensitive: bool = False) -> pd.DataFrame:
    """
    在指定列中搜索匹配模式的行

    :param df: 输入 DataFrame
    :param column: 要搜索的列名
    :param pattern: 搜索模式（支持正则表达式）
    :param case_sensitive: 是否区分大小写，默认 False
    :return: 匹配行的新 DataFrame
    """
    mask = df[column].str.contains(pattern, na=False, case=case_sensitive, regex=True)
    return df[mask].copy()


def describe_dataframe(df: pd.DataFrame, include=None) -> pd.DataFrame:
    """
    生成 DataFrame 的描述性统计摘要

    :param df: 输入 DataFrame
    :param include: 包含的数据类型，默认 None（仅数值列）
    :return: 描述性统计 DataFrame
    """
    return df.describe(include=include).copy()


def create_pivot_table(
    df: pd.DataFrame,
    index,
    columns,
    values,
    aggfunc: str = "mean",
    margins: bool = False,
    margins_name: str = "All",
    sort: bool = False,
) -> pd.DataFrame:
    """
    创建数据透视表

    :param df: 输入 DataFrame
    :param index: 行索引列名（字符串或列表）
    :param columns: 列索引列名（字符串或列表）
    :param values: 值列名（字符串或列表）
    :param aggfunc: 聚合函数，默认 "mean"
    :param margins: 是否显示总计/小计，默认 False
    :param margins_name: 总计/小计行/列的名称，默认 "All"
    :param sort: 是否对结果排序，默认 False
    :return: 透视表 DataFrame
    """
    return pd.pivot_table(
        df,
        index=index,
        columns=columns,
        values=values,
        aggfunc=aggfunc,
        margins=margins,
        margins_name=margins_name,
        sort=sort,
    )


def eval_expression(df: pd.DataFrame, expr: str) -> pd.DataFrame:
    """
    在 DataFrame 上执行表达式计算

    :param df: 输入 DataFrame
    :param expr: 表达式字符串（如 "C = A + B"）
    :return: 执行表达式后的新 DataFrame
    """
    return df.eval(expr).copy()


def filter_by_column_range(
    df: pd.DataFrame,
    column: str,
    min_val=None,
    max_val=None,
) -> pd.DataFrame:
    """
    按列值范围筛选 DataFrame

    :param df: 输入 DataFrame
    :param column: 筛选列名
    :param min_val: 最小值（包含），None 表示不限制下界
    :param max_val: 最大值（包含），None 表示不限制上界
    :return: 筛选后的新 DataFrame
    """
    mask = pd.Series(True, index=df.index)
    if min_val is not None:
        mask = mask & (df[column] >= min_val)
    if max_val is not None:
        mask = mask & (df[column] <= max_val)
    return df[mask].copy()
