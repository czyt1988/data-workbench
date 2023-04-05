import os
from typing import List,Dict,Optional
import pandas as pd

'''
本文件da_打头的变量和函数属于da系统的默认函数，如果改动会导致da系统异常
'''


def da_get_file_read_filters() -> List[str]:
    '''
    获取支持的文件列表
        return list[str]
    '''
    return ['csv (*.csv)','xls (*.xls)']


def read_csv(path:str,args:Optional[Dict] = None):
    if args is None:
        args = {}
    return pd.read_csv(path,**args)

'''
这里是注册后缀对应的处理方式
'''
da_global_reader_dict = {
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

import inspect
import numpy as np
if __name__ == '__main__':
    print(da_get_file_read_filters())
    print(read_csv.__defaults__)
    print('co_argcount=',read_csv.__code__.co_argcount)
    print('co_varnames=',read_csv.__code__.co_varnames)
    df = read_csv(r'F:\work\[07]数据挖掘\查询power.csv')
    print(df.index)
    print(df.index[[1,2,3]])
    print(df.head)
    df.drop(index=df.index[[1,2,3]],axis=0,inplace=True)
    print(df.head)
