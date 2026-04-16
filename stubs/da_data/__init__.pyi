"""
da_data 模块类型定义

此模块提供数据管理功能，包括 DAData 数据包装类、DADataManager 数据管理器以及数据变更类型枚举。
"""

from typing import Any, List, Optional, Union, overload
import pandas as pd


class DAData:
    """
    数据包装类，轻量级包装器，持有 shared_ptr<DAAbstractData>
    
    此类用于包装 pandas DataFrame 或 Series，提供统一的数据访问接口。
    """
    
    @overload
    def __init__(self) -> None:
        """
        默认构造函数
        
        创建一个空的 DAData 对象。
        """
        ...
    
    @overload
    def __init__(self, obj: Any) -> None:
        """
        从 Python 对象构造
        
        Args:
            obj: pandas DataFrame 或 Series 对象
        """
        ...
    
    def __init__(self, obj: Optional[Any] = None) -> None:
        """
        构造函数
        
        Args:
            obj: 可选的 pandas DataFrame 或 Series 对象
        """
        ...
    
    def toPyObject(self) -> Any:
        """
        返回底层 pandas 对象
        
        Returns:
            底层的 pandas DataFrame 或 Series 对象
        """
        ...
    
    def toDataFrame(self) -> pd.DataFrame:
        """
        转换为 DataFrame
        
        Returns:
            转换后的 pandas DataFrame
        """
        ...
    
    def toSeries(self) -> pd.Series:
        """
        转换为 Series
        
        Returns:
            转换后的 pandas Series
        """
        ...
    
    def getName(self) -> str:
        """
        获取数据名称
        
        Returns:
            数据名称字符串
        """
        ...
    
    def setName(self, name: str) -> None:
        """
        设置数据名称
        
        Args:
            name: 数据名称
        """
        ...
    
    def getDescribe(self) -> str:
        """
        获取数据描述
        
        Returns:
            数据描述字符串
        """
        ...
    
    def setDescribe(self, describe: str) -> None:
        """
        设置数据描述
        
        Args:
            describe: 数据描述
        """
        ...
    
    def isNull(self) -> bool:
        """
        检查数据是否为空
        
        Returns:
            True 如果数据为空，否则 False
        """
        ...
    
    def id(self) -> Any:
        """
        获取数据 ID
        
        Returns:
            数据唯一标识符
        """
        ...
    
    def isDataFrame(self) -> bool:
        """
        检查是否为 DataFrame 类型
        
        Returns:
            True 如果是 DataFrame，否则 False
        """
        ...
    
    def isSeries(self) -> bool:
        """
        检查是否为 Series 类型
        
        Returns:
            True 如果是 Series，否则 False
        """
        ...
    
    def setPyObject(self, obj: Any) -> None:
        """
        设置 Python 对象
        
        Args:
            obj: pandas DataFrame 或 Series 对象
        """
        ...
    
    def isHaveDataManager(self) -> bool:
        """
        检查是否有关联的数据管理器
        
        Returns:
            True 如果有关联的数据管理器，否则 False
        """
        ...
    
    def getDataManager(self) -> 'DADataManager':
        """
        获取关联的数据管理器
        
        Returns:
            关联的 DADataManager 对象
        """
        ...


class DataChangeType:
    """
    数据变更类型枚举
    
    定义数据可能发生的变更类型，用于通知 UI 更新。
    """
    
    Name: int
    """名称变更"""
    
    Describe: int
    """描述变更"""
    
    Value: int
    """数值变更"""
    
    ColumnName: int
    """DataFrame 列名变更"""


class DADataManager:
    """
    数据管理器
    
    数据管理中心，提供数据的添加、删除、查找和撤销/重做功能。
    """
    
    def addDataFrame(self, df: Any, name: str) -> None:
        """
        添加 pandas DataFrame 到数据管理器
        
        Args:
            df: pandas DataFrame 对象
            name: 数据名称
        """
        ...
    
    def getDataCount(self) -> int:
        """
        获取数据数量
        
        Returns:
            管理器中数据的数量
        """
        ...
    
    def getDataName(self, index: int) -> str:
        """
        根据索引获取数据名称
        
        Args:
            index: 数据索引
            
        Returns:
            数据名称
        """
        ...
    
    def addData(self, data: DAData) -> None:
        """
        添加 DAData 对象到管理器（无撤销/重做）
        
        Args:
            data: DAData 对象
        """
        ...
    
    def addData_(self, data: DAData) -> None:
        """
        添加 DAData 对象到管理器（支持撤销/重做）
        
        Args:
            data: DAData 对象
        """
        ...
    
    def getAllDatas(self) -> List[DAData]:
        """
        获取所有数据对象
        
        Returns:
            DAData 对象列表
        """
        ...
    
    def findDatas(self, pattern: str, cs: int = 0) -> List[DAData]:
        """
        根据名称模式查找数据
        
        Args:
            pattern: 名称模式字符串
            cs: 大小写敏感选项，0=不敏感，1=敏感
            
        Returns:
            匹配的 DAData 对象列表
        """
        ...
    
    def findDatasReg(self, regex_pattern: str) -> List[DAData]:
        """
        使用正则表达式查找数据
        
        Args:
            regex_pattern: 正则表达式模式
            
        Returns:
            匹配的 DAData 对象列表
        """
        ...
    
    def removeData(self, data: DAData) -> None:
        """
        移除数据（无撤销/重做）
        
        Args:
            data: 要移除的 DAData 对象
        """
        ...
    
    def removeData_(self, data: DAData) -> None:
        """
        移除数据（支持撤销/重做）
        
        Args:
            data: 要移除的 DAData 对象
        """
        ...
    
    def getDataIndex(self, data: DAData) -> int:
        """
        获取数据在管理器中的索引
        
        Args:
            data: DAData 对象
            
        Returns:
            数据索引，如果未找到则返回 -1
        """
        ...
    
    def getData(self, index: int) -> DAData:
        """
        根据索引获取数据
        
        Args:
            index: 数据索引
            
        Returns:
            DAData 对象
        """
        ...
    
    def isDirty(self) -> bool:
        """
        检查管理器是否有未保存的更改
        
        Returns:
            True 如果有未保存的更改，否则 False
        """
        ...
    
    def setDirtyFlag(self, on: bool) -> None:
        """
        设置/清除脏标志
        
        Args:
            on: True 设置脏标志，False 清除脏标志
        """
        ...
    
    def notifyDataChangedSignal(self, data: DAData, changeType: DataChangeType) -> None:
        """
        通知 UI 数据已变更
        
        Args:
            data: 发生变更的 DAData 对象
            changeType: 变更类型，使用 DataChangeType 枚举值
        """
        ...