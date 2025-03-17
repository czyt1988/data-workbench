# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
import pandas as pd
import inspect
import numpy as np
from DAWorkbench.logger import log_function_call  # type: ignore # 引入装饰器
from loguru import logger
import chardet

'''
本文件da_打头的变量和函数属于da系统的默认函数，如果改动会导致da系统异常
'''

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
    '''
    if args is None:
        args = {}
    encoding = detect_encoding(path)
    return pd.read_csv(path,encoding=encoding,**args)

@log_function_call
def read_pkl(path:str,args:Optional[Dict] = None) -> pd.DataFrame:
    '''
    读取pkl文件
    '''
    if args is None:
        args = {}
    df=pd.read_pickle(path,**args)
#    df[0] = pd.to_datetime(df[0], unit='ns')
    
    #判断df的表头是否为str以外的类型，如果不是str类型，转换为str类型（header=None时自动生成的表头是int64,索引的时候使用字符串会报错）
    if df.columns.dtype != np.dtype('<U1'):
        df.columns = df.columns.astype(str)
    return df

@log_function_call
def read_txt(path:str,args:Optional[Dict] = None) -> pd.DataFrame:
    '''
    读取txt文件

    sep 默认‘\t’(制表位)字符串，默认‘\t’(制表位),此外，长度超过1个字符且不同于的分隔符 '\s+' 将被解释为正则表达式，并且还将强制使用Python解析引擎。
    header 指定表头，数据从指定后开始解析，如果没有表头，要设置为None,header=None
    skiprows 指定跳过的内容，第一行为表头，
    skipfooter 文件底部要跳过的行数(引擎=‘c’不支持)。
    nrows 要读取的文件行数。对于读取大文件片段非常有用。
    skip_blank_lines 如果为True，则跳过空行，而不是解释为NaN值
    skipinitialspace 布尔值，默认为False，跳过分隔符后面的空格。
    '''
    if args is None:
        args = {}
    df = pd.read_table(path,**args)
    #判断df的表头是否为str以外的类型，如果不是str类型，转换为str类型（header=None时自动生成的表头是int64,索引的时候使用字符串会报错）
    if df.columns.dtype != np.dtype('<U1'):
        df.columns = df.columns.astype(str)
    return df

@log_function_call
def read_excel(path:str,args:Optional[Dict] = None)-> pd.DataFrame:
    '''
    读取excel文件
    '''
    if args is None:
        args = {}
    return pd.read_excel(path,**args)

@log_function_call
def da_read(path:str,args:Optional[Dict] = None):
    '''
    读取文件
    '''
    suffix = os.path.splitext(path)[-1][1:]
    fun = da_global_reader_dict.get(suffix,None)
    if fun is None:
        return None
    return fun(path,args)


def da_get_file_read_filters() -> List[str]:
    '''
    获取支持的文件列表
        return list[str]
    '''
    return ['all support(*.txt *.csv *.xls *.xlsx *.pkl)',
            'text (*.txt)',
            'csv (*.csv)',
            'xls (*.xls),xlsx (*.xlsx)',
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
}


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
