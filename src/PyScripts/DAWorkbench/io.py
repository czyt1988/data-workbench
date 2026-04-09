# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
import pandas as pd
from pathlib import Path
import inspect
import numpy as np
from DAWorkbench.da_logger import log_function_call  # type: ignore # 引入装饰器
import DAWorkbench.utils as daUtils
from loguru import logger
import chardet
import da_app,da_interface,da_data

'''
本文件da_打头的变量和函数属于da系统的默认函数，如果改动会导致da系统异常
'''

def _try_convert_to_datetime(series: pd.Series, sample_size: int = 100) -> pd.Series:
    '''
    尝试将Series转换为datetime类型
    
    如果Series可以被解析为有效的datetime，则返回转换后的Series，
    否则返回原始Series。
    
    @param series: pd.Series 要尝试转换的Series
    @param sample_size: int 用于检测的样本数量，默认100
    @return: pd.Series 转换后的Series或原始Series
    '''
    # 如果已经是datetime类型，直接返回
    if pd.api.types.is_datetime64_any_dtype(series):
        return series
    
    # 只处理object或string类型的列
    if not (pd.api.types.is_object_dtype(series) or pd.api.types.is_string_dtype(series)):
        return series
    
    # 获取非空样本进行测试
    non_null = series.dropna()
    if len(non_null) == 0:
        return series
    
    sample = non_null.head(sample_size)
    
    try:
        # 尝试转换为datetime
        converted = pd.to_datetime(sample, errors='coerce')
        
        # 检查转换成功率
        # 如果超过80%的值成功转换，则认为这是日期列
        success_rate = converted.notna().sum() / len(sample)
        if success_rate >= 0.8:
            # 对整个列进行转换
            full_converted = pd.to_datetime(series, errors='coerce')
            logger.debug(f"Column '{series.name}' detected as datetime, converted successfully")
            return full_converted
    except Exception as e:
        logger.debug(f"Failed to convert column '{series.name}' to datetime: {e}")
    
    return series

def _auto_detect_datetime_columns(df: pd.DataFrame, sample_size: int = 100) -> pd.DataFrame:
    '''
    自动检测DataFrame中的日期时间列并转换
    
    遍历DataFrame的所有列，尝试检测并转换日期时间列。
    只有当列的数据类型是object或string，且大部分值可以被解析为有效日期时，
    才会进行转换。
    
    @param df: pd.DataFrame 要处理的DataFrame
    @param sample_size: int 用于检测的样本数量，默认100
    @return: pd.DataFrame 处理后的DataFrame
    '''
    for col in df.columns:
        df[col] = _try_convert_to_datetime(df[col], sample_size)
    
    return df

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
        logger.debug("Detection confidence is too low, using default encoding 'utf-8'")
        return 'utf-8'

    return encoding

@log_function_call
def read_csv(path:str,args:Optional[Dict] = None) -> pd.DataFrame:
    '''
    读取csv文件
    @param path: str   文件路径
    @param args: dict   读取参数
    '''
    if args is None:
        args = {}
    
    # 如果用户已经指定了编码，直接使用
    if 'encoding' in args:
        df = pd.read_csv(path, **args)
        df.columns = df.columns.astype(str)
        # 自动检测并转换日期列
        df = _auto_detect_datetime_columns(df)
        return df
    
    # 尝试检测编码
    detected_encoding = detect_encoding(path)
    
    # 编码尝试顺序：检测到的编码优先，然后是常见中文编码
    encodings_to_try = [detected_encoding] if detected_encoding else []
    # 添加常见中文编码作为备选
    fallback_encodings = ['utf-8', 'gbk', 'gb2312', 'gb18030', 'utf-8-sig', 'latin1']
    for enc in fallback_encodings:
        if enc not in encodings_to_try:
            encodings_to_try.append(enc)
    
    last_error = None
    for encoding in encodings_to_try:
        try:
            df = pd.read_csv(path, encoding=encoding, **args)
            df.columns = df.columns.astype(str)
            logger.debug(f"Successfully read CSV with encoding: {encoding}")
            # 自动检测并转换日期列
            df = _auto_detect_datetime_columns(df)
            return df
        except UnicodeDecodeError as e:
            last_error = e
            logger.debug(f"Failed to read CSV with encoding {encoding}: {e}")
            continue
        except Exception as e:
            last_error = e
            logger.debug(f"Error reading CSV with encoding {encoding}: {e}")
            continue
    
    # 所有编码都失败，抛出最后一个错误
    if last_error:
        raise last_error
    raise UnicodeDecodeError('utf-8', b'', 0, 1, f"Failed to read CSV file with all tried encodings: {encodings_to_try}")

@log_function_call
def read_pkl(path:str,args:Optional[Dict] = None) -> pd.DataFrame:
    '''
    读取pkl文件
    @param path: str   文件路径
    @param args: dict   读取参数
    '''
    if args is None:
        args = {}
    df=pd.read_pickle(path,**args)
#    df[0] = pd.to_datetime(df[0], unit='ns')
    
    #判断df的表头是否为str以外的类型，如果不是str类型，转换为str类型（header=None时自动生成的表头是int64,索引的时候使用字符串会报错）
    df.columns = df.columns.astype(str)
    
    # 自动检测并转换日期列
    df = _auto_detect_datetime_columns(df)
    
    return df

@log_function_call
def read_txt(path:str,args:Optional[Dict] = None) -> pd.DataFrame:
    r'''
    读取txt文件

    sep 默认'\t'(制表位)字符串，默认'\t'(制表位),此外，长度超过1个字符且不同于'\s+'的分隔符将被解释为正则表达式，并且还将强制使用Python解析引擎。
    header 指定表头，数据从指定后开始解析，如果没有表头，要设置为None,header=None
    skiprows 指定跳过的内容，第一行为表头，
    skipfooter 文件底部要跳过的行数(引擎='c'不支持)。
    nrows 要读取的文件行数。对于读取大文件片段非常有用。
    skip_blank_lines 如果为True，则跳过空行，而不是解释为NaN值
    skipinitialspace 布尔值，默认为False，跳过分隔符后面的空格。
    '''
    if args is None:
        args = {}
    df = pd.read_table(path,**args)
    #判断df的表头是否为str以外的类型，如果不是str类型，转换为str类型（header=None时自动生成的表头是int64,索引的时候使用字符串会报错）
    df.columns = df.columns.astype(str)
    # 自动检测并转换日期列
    df = _auto_detect_datetime_columns(df)
    return df

@log_function_call
def read_excel(path:str,args:Optional[Dict] = None)-> pd.DataFrame:
    '''
    读取excel文件
    @param path: str   文件路径
    '''
    if args is None:
        args = {}
    df = pd.read_excel(path,**args)
    df.columns = df.columns.astype(str)
    # 自动检测并转换日期列
    df = _auto_detect_datetime_columns(df)
    return df

@log_function_call
def read_parquet(path:str,args:Optional[Dict] = None)-> pd.DataFrame:
    '''
    读取parquet文件
    @param path: str   文件路径
    '''
    if args is None:
        args = {}
    df = pd.read_parquet(path,**args)
    df.columns = df.columns.astype(str)
    # 自动检测并转换日期列
    df = _auto_detect_datetime_columns(df)
    return df

@log_function_call
def da_read(path:str,args:Optional[Dict] = None):
    '''
    读取文件
    @param path: str   文件路径
    '''
    suffix = os.path.splitext(path)[-1][1:]
    fun = da_global_reader_dict.get(suffix,None)
    if fun is None:
        return None
    return fun(path,args)

@log_function_call
def da_read_and_add_to_datamanager(path:str,args:Optional[Dict] = None):
    '''
    读取文件并添加到数据管理器中
    @param path: str   文件路径
    @param args: dict   参数
    '''
    df = da_read(path,args)
    if df is None:
        return
    add_to_datamanager(df,use_redo=True,name=Path(path).stem,describe=path)

def da_get_file_read_filters() -> List[str]:
    '''
    获取支持的文件列表
        return list[str]
    '''
    return ['all support(*.txt *.csv *.xls *.xlsx *.parquet *.pkl)',
            'text (*.txt)',
            'csv (*.csv)',
            'xls (*.xls),xlsx (*.xlsx)',
            'parquet (*.parquet)',
            'pickle (*.pkl)',
            'all(*.*)'
            ]

'''
这里是注册后缀对应的处理方式
注意这个变量一定要在最后写
'''
da_global_reader_dict = {
    'txt':read_txt,
    'csv':read_csv,
    'pkl':read_pkl,
    'xls':read_excel,
    'xlsx':read_excel,
    'parquet':read_parquet
}

def add_to_datamanager(df:pd.DataFrame,use_redo:bool = True,name:str = None,describe:str = None):
    '''
    把数据添加到dataworkbench的数据管理器中
    @param df: pandas.DataFrame 数据
    @param use_redo: bool 是否使用redo/undo
    @param name: str 数据名称
    @param describe: str 数据描述
    '''
    # 把数据添加到dataworkbench的数据管理器中（addData_的下斜杠代表支持redo/undo）
    data = da_data.DAData(df)
    # 把文件名称获取作为数据名称
    data.setName(name)
    data.setDescribe(describe)
    datamanager = da_app.getCore().getDataManagerInterface()
    if use_redo:
        datamanager.addData_(data)
    else:
        datamanager.addData(data)

##################################################

@log_function_call
def da_to_csv(df, path:str,args:Optional[Dict] = None):
    '''
    保存csv文件
    '''
    if args is None:
        args = {}
    df.to_csv(path_or_buf=path,**args)

@log_function_call
def da_to_excel(df, path:str ,args:Optional[Dict] = None):
    '''
    保存csv文件
    '''
    if args is None:
        args = {}
    df.to_excel(path,**args)

@log_function_call
def da_to_pickle(df, path:str ,args:Optional[Dict] = None):
    '''
    保存pickle文件
    '''
    if args is None:
        args = {}
    df.to_pickle(path,**args)

@log_function_call
def da_to_parquet(df, path:str ,args:Optional[Dict] = None):
    '''
    保存parquet文件
    '''
    if args is None:
        args = {}
    df.to_parquet(path,**args)

if __name__ == '__main__':
    print(da_get_file_read_filters())
    print(read_csv.__defaults__)
    print('co_argcount=',read_csv.__code__.co_argcount)
    print('co_varnames=',read_csv.__code__.co_varnames)
    
    da_read(r'C:\src\Qt\pipe-designer-plugin\test-project-files\阀门14-四端参数.xlsx')

    # print(res)
    # v = np.genfromtxt(r'C:\src\Qt\data-workbench\tmp\测试数据.txt',skip_header=14,names=True,encoding='gbk',dtype=float)
    # print(v.shape)
    # print(v.dtype)
    # res = pd.DataFrame(v)
    # print(res)
