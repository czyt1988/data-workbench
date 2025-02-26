# -*- coding: utf-8 -*-
import os
from typing import List, Dict, Optional, Tuple, Literal
import pandas as pd
import numpy as np
from DAWorkbench.logger import log_function_call  # type: ignore # 引入装饰器
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
def da_drop_na(df: pd.DataFrame, axis:int = 0, how:Literal['any','all']='any', index:Optional[List[int]]=None, thresh:int|None=None):
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
    df.dropna(axis=axis,how=how ,subset=subset, thresh=thresh, inplace=True)

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
        df.insert(col,name,np.full(df.shape[0],defaultvalue,dtype=dtype))
    else:
        s = None
        # 对于数值，使用linspace，对于日期使用arange
        if dtype == np.datetime64:
            #日期单独处理
            if stop is not None:
                # 指定了结束日期
                s = pd.date_range(start, stop, periods=df.shape[0])
                print(s)
            else:
                # 没有指定结束日期，就每秒递增
                start = np.datetime64(start)
                step = np.timedelta64(1, 's')
                stop = start + step*df.shape[0]
                s = np.arange(start,stop,step)
        else:
            if stop is not None:
                s = np.linspace(start,stop,df.shape[0])
            else:
                s = np.linspace(start,start+df.shape[0],df.shape[0])
        df.insert(col,name,s)

@log_function_call
def da_itake_column(df: pd.DataFrame, col: int) -> pd.Series:
    '''提取一列

    Args:
        df (pd.DataFrame): DataFrame
        col (int): 列索引
    '''
    s = df[df.columns[col]]
    da_drop_icolumn(df=df,index=[col])
    return s

@log_function_call
def da_insert_at(df: pd.DataFrame, col: int,series:pd.Series):
    df.insert(col,series.name,series)


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
    da_insert_column(df=df, col=2, name='insert-samevalue',defaultvalue=1.0)
    da_insert_column(df=df, col=3, name='insert-datetime',dtype=np.datetime64,start='2020-01-01')
    da_insert_column(df=df, col=3, name='insert-datetime2',dtype=np.datetime64,start='2020-01-01',stop='2021-01-01')
    print(df)


if __name__ == '__main__':
    # __tst_insert_column()
    df = make_dataframe();
    print(df)
    s = da_itake_column(df,1)
    print(df)
    print(s)
    da_insert_at(df,1,s)
    print(df)
