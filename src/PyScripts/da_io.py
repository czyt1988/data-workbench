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
    return ['csv (*.csv)','xls (*.xls)']


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

    这里不使用pd.read_table，因为txt文件读取时，需要指定分隔符，否则会报错,这里使用的是np.genfromtxt,它可以更好的猜测出数据表内容

    delimiter 关键字用于定义应如何进行拆分,假设 delimiter=None ，表示该行沿空白（包括制表符）拆分，连续的空白视为单个空白
    autostrip 默认情况下，当一行分解为一系列字符串时，单个条目不会被去掉前导空格或尾随空格。通过设置可选参数可以覆盖此行为 autostrip 达到一定的价值 True(类似trim效果)
    skip_header,skip_footer 文件中的头可能会妨碍数据处理。在这种情况下，我们需要使用 skip_header 可选参数。在执行任何其他操作之前，此参数的值必须是与要在文件开头跳过的行数相对应的整数。同样，我们可以跳过最后一个 n 通过使用 skip_footer 属性并赋予其值 n 
    usecols 在某些情况下，我们对数据的所有列不感兴趣，只对其中的一些列感兴趣。我们可以选择要用导入的列 usecols 参数。此参数接受与要导入的列的索引对应的单个整数或整数序列。请记住，按照惯例，第一列的索引为0。负整数的行为与常规的python负索引相同。
    names 处理表格数据时的一种自然方法是为每一列分配一个名称,另一个简单的方法是使用 names 具有字符串序列或逗号分隔字符串的关键字：np.genfromtxt(data, names="A, B, C")
        有时我们可能需要从数据本身定义列名。在这种情况下，我们必须使用 names 值为的关键字 True . 然后将从第一行（在 skip_header 一个），即使行被注释掉：
        >>> data = StringIO("So it goes\n#a b c\n1 2 3\n 4 5 6")
        >>> np.genfromtxt(data, skip_header=1, names=True)
        array([(1.0, 2.0, 3.0), (4.0, 5.0, 6.0)],
            dtype=[('a', '<f8'), ('b', '<f8'), ('c', '<f8')])
    '''
    if args is None:
        args = {}
    v = np.genfromtxt(path,**args)
    return pd.DataFrame(v)



'''
这里是注册后缀对应的处理方式
txt 不注册，需要单独处理
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



if __name__ == '__main__':
    print(da_get_file_read_filters())
    print(read_csv.__defaults__)
    print('co_argcount=',read_csv.__code__.co_argcount)
    print('co_varnames=',read_csv.__code__.co_varnames)
    
    # res = pd.read_table(r'C:\src\Qt\data-workbench\tmp\测试数据.txt',skiprows=14,encoding='gbk',nrows=20,dtype=float)
    # print(len(res.columns))
    # print(res.shape)
    # v = np.genfromtxt(r'C:\src\Qt\data-workbench\tmp\测试数据.txt',skip_header=14,names=True,encoding='gbk',dtype=float)
    # print(v.shape)
    # print(v.dtype)
    # res = pd.DataFrame(v)
    # print(res)
