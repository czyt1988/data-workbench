"""
da_data 模块的 Mock 实现

此模块提供数据管理功能的 Mock 实现，包括 DAData 数据包装类、DADataManager 数据管理器以及数据变更类型枚举。
此实现工作在标准 Python 环境中（无需 Qt 或 C++ 程序）。
"""

from typing import Any, List, Optional, Union
import pandas as pd
import re
from enum import IntEnum


class DataChangeType(IntEnum):
    """
    数据变更类型枚举
    
    定义数据可能发生的变更类型，用于通知 UI 更新。
    """
    Name = 0
    """名称变更"""
    
    Describe = 1
    """描述变更"""
    
    Value = 2
    """数值变更"""
    
    ColumnName = 3
    """DataFrame 列名变更"""


class DAData:
    """
    数据包装类，轻量级包装器，持有 pandas DataFrame 或 Series
    
    此类用于包装 pandas DataFrame 或 Series，提供统一的数据访问接口。
    这是一个真正的包装器，不是 Mock。
    """
    
    def __init__(self, obj: Optional[Any] = None) -> None:
        """
        构造函数
        
        Args:
            obj: 可选的 pandas DataFrame 或 Series 对象
        """
        self._obj = obj
        self._name = ""
        self._describe = ""
        self._manager = None
        
    def toPyObject(self) -> Any:
        """
        返回底层 pandas 对象
        
        Returns:
            底层的 pandas DataFrame 或 Series 对象
        """
        return self._obj
    
    def toDataFrame(self) -> pd.DataFrame:
        """
        转换为 DataFrame
        
        Returns:
            转换后的 pandas DataFrame
        
        Raises:
            ValueError: 如果底层对象不是 DataFrame
        """
        if not self.isDataFrame():
            raise ValueError("DAData does not contain a DataFrame")
        return self._obj
    
    def toSeries(self) -> pd.Series:
        """
        转换为 Series
        
        Returns:
            转换后的 pandas Series
        
        Raises:
            ValueError: 如果底层对象不是 Series
        """
        if not self.isSeries():
            raise ValueError("DAData does not contain a Series")
        return self._obj
    
    def getName(self) -> str:
        """
        获取数据名称
        
        Returns:
            数据名称字符串
        """
        return self._name
    
    def setName(self, name: str) -> None:
        """
        设置数据名称
        
        Args:
            name: 数据名称
        """
        self._name = name
        if self._manager:
            self._manager.notifyDataChangedSignal(self, DataChangeType.Name)
    
    def getDescribe(self) -> str:
        """
        获取数据描述
        
        Returns:
            数据描述字符串
        """
        return self._describe
    
    def setDescribe(self, describe: str) -> None:
        """
        设置数据描述
        
        Args:
            describe: 数据描述
        """
        self._describe = describe
        if self._manager:
            self._manager.notifyDataChangedSignal(self, DataChangeType.Describe)
    
    def isNull(self) -> bool:
        """
        检查数据是否为空
        
        Returns:
            True 如果数据为空，否则 False
        """
        return self._obj is None
    
    def id(self) -> int:
        """
        获取数据 ID
        
        Returns:
            数据唯一标识符（使用 Python 内置 id 函数）
        """
        return id(self)
    
    def isDataFrame(self) -> bool:
        """
        检查是否为 DataFrame 类型
        
        Returns:
            True 如果是 DataFrame，否则 False
        """
        return isinstance(self._obj, pd.DataFrame)
    
    def isSeries(self) -> bool:
        """
        检查是否为 Series 类型
        
        Returns:
            True 如果是 Series，否则 False
        """
        return isinstance(self._obj, pd.Series)
    
    def setPyObject(self, obj: Any) -> None:
        """
        设置 Python 对象
        
        Args:
            obj: pandas DataFrame 或 Series 对象
        """
        self._obj = obj
        if self._manager:
            self._manager.notifyDataChangedSignal(self, DataChangeType.Value)
    
    def isHaveDataManager(self) -> bool:
        """
        检查是否有关联的数据管理器
        
        Returns:
            True 如果有关联的数据管理器，否则 False
        """
        return self._manager is not None
    
    def getDataManager(self) -> Optional['DADataManager']:
        """
        获取关联的数据管理器
        
        Returns:
            关联的 DADataManager 对象，如果没有则返回 None
        """
        return self._manager


class DADataManager:
    """
    数据管理器
    
    数据管理中心，提供数据的添加、删除、查找功能。
    这是一个 Mock 实现，使用 Python list 模拟数据存储，没有真正的撤销/重做栈。
    """
    
    def __init__(self) -> None:
        """
        构造函数
        """
        self._datas: List[DAData] = []
        self._dirty = False
    
    def addDataFrame(self, df: Any, name: str) -> None:
        """
        添加 pandas DataFrame 到数据管理器
        
        Args:
            df: pandas DataFrame 对象
            name: 数据名称
        """
        data = DAData(df)
        data.setName(name)
        self.addData(data)
    
    def getDataCount(self) -> int:
        """
        获取数据数量
        
        Returns:
            管理器中数据的数量
        """
        return len(self._datas)
    
    def getDataName(self, index: int) -> str:
        """
        根据索引获取数据名称
        
        Args:
            index: 数据索引
            
        Returns:
            数据名称
        """
        if 0 <= index < len(self._datas):
            return self._datas[index].getName()
        return ""
    
    def addData(self, data: DAData) -> None:
        """
        添加 DAData 对象到管理器（无撤销/重做）
        
        Args:
            data: DAData 对象
        """
        data._manager = self
        self._datas.append(data)
        self.setDirtyFlag(True)
    
    def addData_(self, data: DAData) -> None:
        """
        添加 DAData 对象到管理器（支持撤销/重做）
        
        Args:
            data: DAData 对象
        """
        # Mock 实现中，addData_ 和 addData 相同
        self.addData(data)
    
    def getAllDatas(self) -> List[DAData]:
        """
        获取所有数据对象
        
        Returns:
            DAData 对象列表
        """
        return self._datas.copy()
    
    def findDatas(self, pattern: str, cs: int = 0) -> List[DAData]:
        """
        根据名称模式查找数据
        
        Args:
            pattern: 名称模式字符串
            cs: 大小写敏感选项，0=不敏感，1=敏感
            
        Returns:
            匹配的 DAData 对象列表
        """
        result = []
        flags = 0 if cs == 1 else re.IGNORECASE
        
        for data in self._datas:
            name = data.getName()
            if re.search(pattern, name, flags):
                result.append(data)
        
        return result
    
    def findDatasReg(self, regex_pattern: str) -> List[DAData]:
        """
        使用正则表达式查找数据
        
        Args:
            regex_pattern: 正则表达式模式
            
        Returns:
            匹配的 DAData 对象列表
        """
        result = []
        
        for data in self._datas:
            name = data.getName()
            if re.search(regex_pattern, name):
                result.append(data)
        
        return result
    
    def removeData(self, data: DAData) -> None:
        """
        移除数据（无撤销/重做）
        
        Args:
            data: 要移除的 DAData 对象
        """
        if data in self._datas:
            data._manager = None
            self._datas.remove(data)
            self.setDirtyFlag(True)
    
    def removeData_(self, data: DAData) -> None:
        """
        移除数据（支持撤销/重做）
        
        Args:
            data: 要移除的 DAData 对象
        """
        # Mock 实现中，removeData_ 和 removeData 相同
        self.removeData(data)
    
    def getDataIndex(self, data: DAData) -> int:
        """
        获取数据在管理器中的索引
        
        Args:
            data: DAData 对象
            
        Returns:
            数据索引，如果未找到则返回 -1
        """
        try:
            return self._datas.index(data)
        except ValueError:
            return -1
    
    def getData(self, index: int) -> DAData:
        """
        根据索引获取数据
        
        Args:
            index: 数据索引
            
        Returns:
            DAData 对象
        """
        if 0 <= index < len(self._datas):
            return self._datas[index]
        raise IndexError(f"Index {index} out of range for DADataManager with {len(self._datas)} items")
    
    def isDirty(self) -> bool:
        """
        检查管理器是否有未保存的更改
        
        Returns:
            True 如果有未保存的更改，否则 False
        """
        return self._dirty
    
    def setDirtyFlag(self, on: bool) -> None:
        """
        设置/清除脏标志
        
        Args:
            on: True 设置脏标志，False 清除脏标志
        """
        self._dirty = on
    
    def notifyDataChangedSignal(self, data: DAData, changeType: DataChangeType) -> None:
        """
        通知 UI 数据已变更
        
        Args:
            data: 发生变更的 DAData 对象
            changeType: 变更类型，使用 DataChangeType 枚举值
        """
        # Mock 实现中，打印日志而不是发送真正的信号
        change_type_names = {
            DataChangeType.Name: "Name",
            DataChangeType.Describe: "Describe", 
            DataChangeType.Value: "Value",
            DataChangeType.ColumnName: "ColumnName"
        }
        print(f"[Mock] Data changed: {data.getName()} ({change_type_names.get(changeType, 'Unknown')})")