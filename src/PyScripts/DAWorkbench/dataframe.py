# -*- coding: utf-8 -*-
import os
import sys
from typing import List, Dict, Optional, Union
# 根据Python版本动态导入Literal
if sys.version_info >= (3, 8):
    from typing import Literal
else:
    from typing_extensions import Literal
# 在Python 3.10及之后的版本中引入了|操作符用于类型联合，而在此之前的版本并不支持这种语法,为了支持win7（python3.7）,
# 这里不应使用|操作符可以使用Union替换|，Literal是3.8之后才支持，这里也不应该引入
# 如果确实是在Python 3.7环境下并且需要使用Literal，可以引入typing_extensions包，from typing_extensions import Literal

from pandas._typing import Axis, Scalar
import pandas as pd
import numpy as np
from DAWorkbench.DALogger import log_function_call  # type: ignore # 引入装饰器
import copy

'''
本文件da_打头的变量和函数属于da系统的默认函数，如果改动会导致da系统异常

此文件封装dataframe的操作
'''


@log_function_call
def da_drop_irow(df: pd.DataFrame, index: List[int]):
    '''
    传入行索引，并把对应的行删除
    :param df: pd.DataFrame
    :param index: 删除的行索引
    :return: 此函数不返回值，直接改变df
    '''
    i = df.index[index]
    df.drop(index=i, axis=0, inplace=True)


@log_function_call
def da_drop_icolumn(df: pd.DataFrame, index: List[int]):
    '''
    传入列索引，并把对应的列删除
    :param df:  pd.DataFrame
    :param index: 删除的列索引
    :return: 此函数不返回值，直接改变df
    '''
    cols = df.columns[index]
    df.drop(cols, axis=1, inplace=True)


@log_function_call
def da_drop_na(df: pd.DataFrame, axis: int = 0, how: Literal['any', 'all'] = 'any', index: Optional[List[int]] = None, thresh: Optional[int] = None):
    '''
    删除dataframe中的nan值
    :param df: pd.DataFrame
    :param axis: 0为行，1为列
    :param how: any为只要有nan就删除，all为全为nan才删除
    :param index: 列索引
    :param thresh: 可选参数，表示在删除之前需要满足的非缺失值的最小数量。如果行或列中的非缺失值数量小于等于thresh，则会被删除
    :return: 此函数不返回值，直接改变df
    '''
    subset = None
    if index is not None:
        if axis == 0:
            subset = df.columns[index]
        else:
            subset = df.index[index]
    if thresh is not None:
        df.dropna(axis=axis, subset=subset, thresh=thresh, inplace=True)
    else:
        df.dropna(axis=axis, how=how, subset=subset, inplace=True)

@log_function_call
def da_drop_duplicates(df: pd.DataFrame, keep: Literal['first', 'last'] = 'first', index: Optional[List[int]] = None, ignore_index = False):
    '''
    删除dataframe中的重复值
    :param df: pd.DataFrame
    :param axis: 0为行，1为列
    :param keep: first表示保留第一个重复值，last表示保留最后一个重复值，默认为first
    :param index: 列索引
    :param ignore_index: 是否忽略索引，默认为False
    :return: 此函数不返回值，直接改变df
    '''
    subset = None
    if index is not None:
        subset = df.columns[index].tolist()
    df.drop_duplicates(subset=subset,keep=keep,ignore_index=ignore_index,inplace=True)


@log_function_call
def da_fill_na(df: pd.DataFrame, value: Union[Scalar, dict, None] = None, axis: Optional[Axis] = None, limit: Optional[int] = None, downcast: Optional[dict] = None):
    '''
    填充dataframe中的nan值
    :param df: pd.DataFrame
    :param value: 用于填充缺失值的值或字典。可以是一个标量值、字典、Series 或 DataFrame
    :param axis: 填充的轴方向，0 或 'index' 表示按行填充，1 或 'columns' 表示按列填充。
    :param limit: 限制向前或向后填充的最大数量
    :param downcast:可选的字典，指定向下转型操作（例如将浮点数转换为整数等）。
    :return: 此函数不返回值，直接改变df
    :example:
    df = pd.DataFrame([[np.nan, 2, np.nan, 0],
                    [3, 4, np.nan, 1],
                    [np.nan, np.nan, np.nan, np.nan],
                    [np.nan, 3, np.nan, 4]],
                    columns=list("ABCD"))
    values = {"A": 0, "B": 1, "C": 2, "D": 3}
    df.fillna(value=values)
    '''
    df.fillna(axis=axis, value=value, downcast=downcast, inplace=True)


@log_function_call
def da_ffill_na(df: pd.DataFrame, axis: Optional[Axis] = None, limit: Optional[int] = None, downcast: Optional[dict] = None):
    '''
    填充dataframe中的nan值
    :param df: pd.DataFrame
    :param axis: 填充的轴方向，0 或 'index' 表示按行向下填充，1 或 'columns' 表示按列向右填充。
    :param limit: 限制向后填充的最大数量
    :param downcast:可选的字典，指定向下转型操作（例如将浮点数转换为整数等）。
    :return: 此函数不返回值，直接改变df
    '''
    df.ffill(axis=axis, inplace=True, limit=limit, downcast=downcast)


@log_function_call
def da_bfill_na(df: pd.DataFrame, axis: Optional[Axis] = None, limit: Optional[int] = None, limit_area: Optional[Literal['inside', 'outside']] = None):
    '''
    填充dataframe中的nan值
    :param df: pd.DataFrame
    :param axis: 填充的轴方向,0表示按行向上填充,1表示按列向左填充。
    :param limit: 限制向前填充的最大数量
    :param limit_area: 字符串，默认为 None。可选值为 None、inside 或 outside。inside 表示只填充被有效值包围的缺失值，outside 表示只填充在有效值之前或之后的缺失值。
    :return: 此函数不返回值，直接改变df
    '''
    df.bfill(axis=axis, inplace=True, limit=limit, limit_area=limit_area)


@log_function_call
def da_nstd_filter_outlier(df: pd.DataFrame, n=3, axis: Optional[int] = None, index: Optional[List[int]] = None):
    """
    使用n倍标准差法过滤DataFrame的行或列（直接在原数据上修改）
    :param df: 输入的pd.DataFrame
    :param n: 标准差的倍数，范围是0.1~10，默认为3
    :param axis: 过滤方向，1表示基于选中列过滤行(默认)，0表示基于选中行过滤列
    :param index: 需要过滤的行或列的索引列表，如果为None，则不进行过滤
    :return: 过滤后的DataFrame
    """
    # 如果没有指定要过滤的列/行，直接返回原DataFrame
    if index is None or len(index) == 0:
        return df
        
    # 检查所选列是否全是字符串类型，如果是则跳过计算
    selected_columns = df.iloc[:, index]
    if all(selected_columns.dtypes == "object"):  # Pandas 中字符串列的类型通常是 'object'
        return df
    
    # 计算均值和标准差
    if axis == 1:  # 计算选中列的值，过滤行
        data = df.iloc[:, index]
        mean = data.mean(axis=0)  # 沿着行的方向，计算每一列的平均值
        std = data.std(axis=0)    # 沿着行的方向，计算每一列的标准差

        # 为每一列创建掩码，标识该列中哪些行在均值±n*标准差范围内
        # 初始化一个全True的掩码数组
        keep_rows = pd.Series([True] * df.shape[0], index=df.index)

        # 检查每一列，确定哪些行需要保留
        for i, col_idx in enumerate(index):
            col_data = df.iloc[:, col_idx]
            col_mean = mean.iloc[i]
            col_std = std.iloc[i]
            col_lower = col_mean - n * col_std
            col_upper = col_mean + n * col_std

            # 更新keep_rows，只保留在所有选中列都在范围内的行
            keep_rows = keep_rows & (col_data >= col_lower) & (
                col_data <= col_upper)

        # 直接删除不符合条件的行
        df.drop(df.index[~keep_rows], inplace=True)


@log_function_call
def da_fill_interpolate(df: pd.DataFrame, method: Literal['spline', 'polynomial'] = 'spline', order: int = 1, axis: int = 0, limit: Optional[int] = None,
                        limit_direction: Literal["forward", "backward", "both"] = "forward", limit_area: Literal["inside", "outside", None] = None, downcast: Optional[dict] = None):
    '''
    插值法填充dataframe中的nan值
    :param df: pd.DataFrame。
    :param method: 用于插值法填充缺失值的方法。spline代表线程插值，polynomial代表多项式插值。
    :param order: 插值多项式的次数。
    :param axis: 填充的轴方向，0 或 'index' 表示按行填充，1 或 'columns' 表示按列填充。
    :param limit: 限制向前或向后填充的最大数量
    :param limit_direction: 限制插值的方向，默认为'forward'。
    :param limit_area: 限制插值的区域，在有效值的内部或外部，默认为None,。
    :param downcast:可选的字典，指定向下转型操作（例如将浮点数转换为整数等）。
    :return: 此函数不返回值，直接改变df
    '''
    df.interpolate(method=method, order=order, axis=axis,
                   downcast=downcast, inplace=True)


@log_function_call
def da_clip_outlier(df: pd.DataFrame, lower: Optional[float] = None, upper: Optional[float] = None, axis: int = 0):
    '''
    替换dataframe的异常值
    :param df: pd.DataFrame。
    :param lower: 所有小于 lower 的值会被替换为 lower。如果为 None，则不应用下界。
    :param upper: 所有大于 upper 的值会被替换为 upper。如果为 None，则不应用上界。
    :param axis: 填充的轴方向，0 或 'index' 表示按行填充，1 或 'columns' 表示按列填充。
    :return: 此函数不返回值，直接改变df
    '''
    df.clip(lower=lower, upper=upper, axis=axis, inplace=True)


@log_function_call
def da_create_pivot_table(df: pd.DataFrame, values=None, index=None, columns=None, aggfunc: Literal['mean', 'sum', 'size'] = 'mean',
                          margins: bool = False, margins_name: str = "All", sort: bool = False):
    '''
    创建数据透视表
    :param values: 要进行汇总的数据值
    :param index: 确定行参数
    :param columns: 确定列参数
    :param aggfunc: 要计算的函数，mean求均值、sum求和、size计算个数
    :param margins: 行列数据的统计
    :param margins_name: 行列数据的统计表头名
    :param sort: 聚合后的结果排序
    :return: 数据聚合后的DataFrame
    '''
    return df.pivot_table(values=values, index=index, columns=columns, aggfunc=aggfunc, margins=margins, margins_name=margins_name, sort=sort)


@log_function_call
def da_eval_datas(df: pd.DataFrame, expr:str,parser: str = "pandas", engine: Optional[str] = None,local_dict=None,global_dict=None,resolvers=(),level: int = 0,target=None):
    '''
    pandas.DataFrame.eval的wrapper
    '''
    df.eval(expr ,inplace=True)

@log_function_call
def da_query_datas(df: pd.DataFrame, expr:Optional[str]):
    '''
    pandas.DataFrame.query的wrapper
    :param df: pd.DataFrame。
    :param expr:字符串形式的筛选条件。
    :return: 此函数不返回值，直接改变df
    '''
    df.query(expr ,inplace=True)

@log_function_call
def da_search_data(df: pd.DataFrame, data, start_row=None, start_col=None):
    """
    在DataFrame中查找匹配数据的单元格位置，返回整数索引
    
    参数:
        df: 目标DataFrame
        data: 要匹配的数据（任意类型）
        start_row: 起始行号(整数索引，None表示从0开始)
        start_col: 起始列号(整数索引，None表示从0开始)
    
    返回:
        list: 匹配位置的列表，每个元素为元组 (行索引, 列索引)
    """
    # 处理起始位置
    start_row = start_row or 0
    start_col = start_col or 0
    
    # 确保起始位置在有效范围内
    start_row = max(0, min(start_row, len(df)-1))
    start_col = max(0, min(start_col, len(df.columns)-1))
    
    matches = []
    (rowcnt,colcnt) = df.shape
    # 遍历行和列
    for i in range(start_row, rowcnt):
        for j in range(start_col, colcnt):
            cell_value = df.iat[i, j]
            
            # 处理NaN/None的特殊比较
            if pd.isna(data):
                if pd.isna(cell_value):
                    matches.append((i, j))
            # 处理其他数据类型的比较
            elif cell_value == data:
                matches.append((i, j))
    
    return matches

@log_function_call
def da_data_select(df: pd.DataFrame, index: str, lower: Optional[float] = None,upper: Optional[float] = None):
    '''
    :param df: pd.DataFrame
    :param lower: 范围下界，None表示无下限
    :param upper: 范围上界，None表示无上限
    :param index: 选中的列索引
    '''
    # 参数校验
    if lower is None and upper is None:
        raise ValueError("必须指定lower或upper至少一个条件")
    
    # 创建条件掩码（初始为全True）
    mask = pd.Series(True, index=df.index)
    
    # 在选中的列上应用范围条件
    col_series = df[index]
    if lower is not None and upper is not None:
        mask &= col_series.between(lower, upper)
    elif lower is not None:
        mask &= (col_series >= lower)
    elif upper is not None:
        mask &= (col_series <= upper)
    
    # 直接删除不符合条件的行
    df.drop(index=df[~mask].index, inplace=True)

@log_function_call
def da_sort(df: pd.DataFrame, by: str, ascending: bool):
    '''
    替换dataframe的异常值
    :param df: pd.DataFrame。
    :param by:数据排序依据。
    :param ascending:数据排序方式。
    :return: 此函数不返回值，直接改变df
    '''
    df.sort_values(by = by ,ascending = ascending, inplace = True)

@log_function_call
def da_to_csv(df: pd.DataFrame, path: str, sep: str):
    '''
    把dataframe写到文件
    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值
    '''
    df.to_csv(path, sep = sep, index = False)

@log_function_call
def da_to_excel(df: pd.DataFrame, path: str):
    '''
    把dataframe写到文件
    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值
    '''
    df.to_excel(path, index = False)

@log_function_call
def da_to_pickle(df: pd.DataFrame, path: str):
    '''
    把dataframe写到文件
    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值
    '''
    df.to_pickle(path)

@log_function_call
def da_to_parquet(df: pd.DataFrame, path: str):
    '''
    把dataframe写到文件
    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值
    '''
    df.to_parquet(path)

@log_function_call
def da_from_pickle(df: pd.DataFrame, path: str):
    '''
    从文件加载到dataframe
    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值，直接改变df
    '''
    tmp = pd.read_pickle(path)
    df.__init__(tmp)

@log_function_call
def da_from_parquet(df: pd.DataFrame, path: str):
    '''
    从文件加载到dataframe
    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值，直接改变df
    '''
    tmp = pd.read_parquet(path)
    df.__init__(tmp)

@log_function_call
def da_astype(df: pd.DataFrame, colsIndex: List[int], dtype: np.dtype):
    '''
    通过列索引改变dataframe的数据类型
    :param df: pd.DataFrame
    :param colsIndex: 列索引
    :param dtype: 类型
    :return: 此函数不返回值，直接改变df
    '''
    cols = [df.columns[v] for v in colsIndex]
    df[cols] = df[cols].astype(dtype)


@log_function_call
def da_setnan(df: pd.DataFrame, rowindex: List[int], colindex: List[int]):
    '''
    设置nan值
    :param df: pd.DataFrame
    :param rowindex: 行索引
    :param colindex: 列索引
    :return: 此函数不返回值，直接改变df
    '''
    if len(rowindex) != len(colindex):
        raise Exception('row length len not equal column index length')
    for r, c in zip(rowindex, colindex):
        df.iat[r, c] = np.nan


@log_function_call
def da_cast_to_num(df: pd.DataFrame,
                   colsIndex: List[int],
                   errors: str = 'coerce',
                   downcast: Optional[str] = None):
    '''
    转换为数值
        errors = {'ignore', 'raise', 'coerce'},
    '''
    cols = [df.columns[v] for v in colsIndex]
    for col in cols:
        df[col] = pd.to_numeric(
            df[col], errors=errors, downcast=downcast)


@log_function_call
def da_cast_to_datetime(df: pd.DataFrame,
                        colsIndex: List[int],
                        errors: str = 'coerce',
                        dayfirst: bool = False,
                        yearfirst: bool = False,
                        utc: Optional[bool] = None,
                        format: Optional[str] = None,
                        exact: bool = True,
                        unit: Optional[str] = 'ns',
                        infer_datetime_format: bool = False,
                        origin: str = 'unix',
                        cache: bool = True
                        ):
    '''
    转换为日期
        errors = {'ignore', 'raise', 'coerce'},
        format
    '''
    cols = [df.columns[v] for v in colsIndex]
    for col in cols:
        df[col] = pd.to_datetime(
            df[col], errors=errors, dayfirst=dayfirst, yearfirst=yearfirst, utc=utc,
            format=format, exact=exact,
            unit=unit, infer_datetime_format=infer_datetime_format,
            origin=origin, cache=cache)


@log_function_call
def da_setindex(df: pd.DataFrame, colsIndex: List[int]):
    '''
    设置索引
    :param df:
    :param colsIndex:
    :return:
    '''
    cols = [df.columns[v] for v in colsIndex]
    index_ser = df.index
    # 这时要把index设置到列里面
    if len(index_ser.names) > 1:
        # 多重索引
        dfindex = index_ser.to_frame()
        for name in index_ser.names:
            cnt = 1
            cn = name
            while cn in df.columns:
                cn = f'{name}_{cnt}'
                cnt += 1
            df[cn] = dfindex[name]
    else:
        cnt = 1
        while index_ser.name in df.columns:
            index_ser.name = f'{index_ser.name}_{cnt}'
            cnt += 1
        df[index_ser.name] = index_ser
    # 把列转换为index
    df.set_index(cols, inplace=True)


@log_function_call
def da_insert_nanrow(df: pd.DataFrame, row: int):
    '''
    插入一行，插入的行默认为nan
    :param df:
    :param row:
    :return:
    '''
    dfindex = df.index
    cols = df.columns
    df1 = df.iloc[:row]
    df2 = df.iloc[row:]
    dfnanrow = pd.DataFrame(df.iloc[row, :]).T
    dfnanrow.at[:, :] = np.nan
    if isinstance(dfindex, pd.RangeIndex):
        df.__init__(pd.concat([df1, dfnanrow, df2], ignore_index=True))
    else:
        df.__init__(pd.concat([df1, dfnanrow, df2], ignore_index=False))


@log_function_call
def da_insert_column(df: pd.DataFrame, col: int, name: str,
                     dtype: Optional[np.dtype] = None,
                     defaultvalue=np.nan,
                     start=None, stop=None):
    '''插入一列，插入的列默认为nan

    Args:
        df (pd.DataFrame): 数据
        col (int): 列号
        name (str): 列名
        dtype (np.dtype, optional):dtype,对于日期，明确dtype才能正确处理. Defaults to None.
        defaultvalue ([type], optional): 默认值，如果设置stop将不起作用. Defaults to np.nan.
        start ([type], optional): 使用np.arange/np.linspace生成列，必须和stop搭配 Defaults to None.
        stop ([type], optional): [description]. Defaults to None.
    '''
    if start is None:
        df.insert(col, name, np.full(df.shape[0], defaultvalue, dtype=dtype))
    else:
        s = None
        # 对于数值，使用linspace，对于日期使用arange
        if dtype == np.datetime64:
            # 日期单独处理
            if stop is not None:
                # 指定了结束日期
                s = pd.date_range(start, stop, periods=df.shape[0])
                print(s)
            else:
                # 没有指定结束日期，就每秒递增
                start = np.datetime64(start)
                step = np.timedelta64(1, 's')
                stop = start + step*df.shape[0]
                s = np.arange(start, stop, step)
        else:
            if stop is not None:
                s = np.linspace(start, stop, df.shape[0])
            else:
                s = np.linspace(start, start+df.shape[0], df.shape[0])
        df.insert(col, name, s)


@log_function_call
def da_itake_column(df: pd.DataFrame, col: int) -> pd.Series:
    '''提取一列

    Args:
        df (pd.DataFrame): DataFrame
        col (int): 列索引
    '''
    s = df[df.columns[col]]
    da_drop_icolumn(df=df, index=[col])
    return s


@log_function_call
def da_insert_at(df: pd.DataFrame, col: int, series: pd.Series):
    df.insert(col, series.name, series)


def make_dataframe(size: int = 100) -> pd.DataFrame:
    '''构建一个数据类型较全面的dataframe

    Args:
        size (int, optional): [description]. Defaults to 100.

    Returns:
        (pd.DataFrame): [description]
    '''
    df = pd.DataFrame()
    dt = np.timedelta64(1, 's')
    df['datatime'] = np.arange(np.datetime64(
        '2021-01-01 00'), np.datetime64('2021-01-01 00')+dt*size, dt, dtype=np.datetime64)
    df['int64'] = np.linspace(-10, 10, size, dtype=np.int64)
    df['uint64'] = np.linspace(0, size, size, dtype=np.uint64)
    df['float32'] = np.linspace(0, size, size, dtype=np.float32)
    df['float32'] = np.sin(df['float32'])
    df['float64'] = np.linspace(0, size, size, dtype=np.float64)
    df['float64'] = np.cos(df['float64'])
    df['str'] = np.array(['text']*size, dtype=np.str_)
    df['complex128'] = np.fft.fft(df['float64'])
    return df


def __tst_pickle():
    df = pd.read_csv(r'F:\work\[07]数据挖掘\查询power.csv')
    path = f'./{id(df)}.pkl'
    da_to_pickle(df, path)
    print('df id =', id(df))
    index = [1, 2, 3]
    print(df)
    bed = da_drop_irow(df, [1, 2, 3])
    da_from_pickle(df, path)
    print(df)
    print('df id =', id(df))


def __tst_drop():
    df = pd.read_csv(r'F:\work\[07]数据挖掘\查询power.csv')
    print(df)
    print(df.columns)
    da_drop_icolumn(df, [1, 2, 3])
    print(df.columns)
    da_drop_irow(df, [1, 2, 3])
    print(df)


def __tst_astype():
    df = pd.read_csv(r'F:\work\[07]数据挖掘\查询power.csv')
    print(df.dtypes)
    da_astype(df, [5, 6, 7, 8], "float32")
    print(df.dtypes)


def __tst_setnan():
    df = pd.read_csv(r'F:\work\[07]数据挖掘\查询power.csv')
    print(df)
    da_setnan(df, [0, 1, 2, 3], [0, 1, 2, 3])
    print(df)


def __tst_setnan():
    df = pd.DataFrame(np.arange(16).reshape((4, 4)), index=[
                      'a', 'b', 'c', 'd'], columns=['1', '2', '3', '4'])
    print(df)
    da_setnan(df, [0, 1, 2, 3], [0, 1, 2, 3])
    print(df)


def __tst_insert_nanrow():
    df = make_dataframe()
    print(df.dtypes)
    da_insert_nanrow(df, 1)
    print(df.dtypes)
    print(df)

    df = make_dataframe()
    print(df.dtypes)
    da_insert_nanrow(df, 1)
    print(df.dtypes)
    print(df)


def __tst_insert_column():
    df = make_dataframe(7)
    print(df)
    da_insert_column(df=df, col=1, name='insert-nan')
    da_insert_column(df=df, col=2, name='insert-samevalue', defaultvalue=1.0)
    da_insert_column(df=df, col=3, name='insert-datetime',
                     dtype=np.datetime64, start='2020-01-01')
    da_insert_column(df=df, col=3, name='insert-datetime2',
                     dtype=np.datetime64, start='2020-01-01', stop='2021-01-01')
    print(df)


def __tst_fill_na():
    df = pd.DataFrame([[np.nan, 2, np.nan, 0],
                       [3, 4, np.nan, 1],
                       [np.nan, np.nan, np.nan, np.nan],
                       [np.nan, 3, np.nan, 4]],
                      columns=list("ABCD"))
    print(df)
    values = {"A": 0, "B": 1, "C": 2, "D": 3}
    da_fill_na(df, value=values)
    print(df)


if __name__ == '__main__':
    __tst_fill_na()
