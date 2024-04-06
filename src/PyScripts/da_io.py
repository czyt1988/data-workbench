import os
from typing import List,Dict,Optional
import pandas as pd
import inspect
import numpy as np

'''
本文件da_打头的变量和函数属于da系统的默认函数，如果改动会导致da系统异常
'''


def da_get_file_read_filters() -> List[str]:
    '''
    获取支持的文件列表
        return list[str]
    '''
    return ['text (*.txt)','csv (*.csv)','xls (*.xls)']


def read_csv(path:str,args:Optional[Dict] = None) -> pd.DataFrame:
    '''
    读取csv文件
    '''
    if args is None:
        args = {}
    return pd.read_csv(path,**args)

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
    return pd.read_table(path,**args)



'''
这里是注册后缀对应的处理方式
txt 不注册，需要单独处理
'''
da_global_reader_dict = {
    'txt':read_txt,
    'csv':read_csv
}

def da_read(path:str,args:Optional[Dict] = None):
    '''
    读取文件
    '''
    suffix = os.path.splitext(path)[-1][1:]
    fun = da_global_reader_dict.get(suffix,None)
    if fun is None:
        return None
    return fun(path,args)



if __name__ == '__main__':
    print(da_get_file_read_filters())
    print(read_csv.__defaults__)
    print('co_argcount=',read_csv.__code__.co_argcount)
    print('co_varnames=',read_csv.__code__.co_varnames)
    
    res = pd.read_table(r'C:\src\Qt\data-workbench\tmp\测试数据.txt',header=14,skiprows=0,encoding='gbk',nrows=20,sep="\s+")
    print(len(res.columns))
    print(res.columns)
    print(res.shape)
    print(res)
    # v = np.genfromtxt(r'C:\src\Qt\data-workbench\tmp\测试数据.txt',skip_header=14,names=True,encoding='gbk',dtype=float)
    # print(v.shape)
    # print(v.dtype)
    # res = pd.DataFrame(v)
    # print(res)
