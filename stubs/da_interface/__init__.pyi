"""
da_interface 模块类型定义

此模块提供 Qt 应用程序核心接口，包括数据管理、UI 操作、命令执行和线程调度等功能。
"""

from typing import Any, Callable, Dict, List, Optional
import pandas as pd
from da_data import DAData, DataChangeType, DADataManager


class DAPythonSignalHandler:
    """
    Python 信号处理器
    
    此类用于在 Qt 主线程中调度 Python 函数的执行。
    """
    
    def callInMainThread(self, func: Callable) -> None:
        """
        在 Qt 主线程中调度 Python 函数
        
        此方法将 Python 函数调度到 Qt 主线程中执行，确保线程安全。
        
        Args:
            func: 要执行的 Python 可调用对象
        """
        ...


class DADataManagerInterface:
    """
    数据管理器接口
    
    提供对应用程序数据管理器的访问，支持数据的增删改查操作。
    """
    
    def addData(self, data: DAData) -> None:
        """
        立即添加数据
        
        将数据添加到数据管理器中，不记录撤销/重做操作。
        
        Args:
            data: 要添加的 DAData 对象
        """
        ...
    
    def addData_(self, data: DAData) -> None:
        """
        添加数据（支持撤销/重做）
        
        将数据添加到数据管理器中，并记录撤销/重做操作。
        
        Args:
            data: 要添加的 DAData 对象
        """
        ...
    
    def removeData(self, data: DAData) -> None:
        """
        立即删除数据
        
        从数据管理器中删除数据，不记录撤销/重做操作。
        
        Args:
            data: 要删除的 DAData 对象
        """
        ...
    
    def removeData_(self, data: DAData) -> None:
        """
        删除数据（支持撤销/重做）
        
        从数据管理器中删除数据，并记录撤销/重做操作。
        
        Args:
            data: 要删除的 DAData 对象
        """
        ...
    
    def getDataCount(self) -> int:
        """
        获取数据数量
        
        Returns:
            数据管理器中的数据总数
        """
        ...
    
    def getData(self, index: int) -> DAData:
        """
        根据索引获取数据
        
        Args:
            index: 数据索引
            
        Returns:
            指定索引处的 DAData 对象
        """
        ...
    
    def getDataIndex(self, data: DAData) -> int:
        """
        获取数据索引
        
        Args:
            data: DAData 对象
            
        Returns:
            数据在管理器中的索引，如果未找到则返回 -1
        """
        ...
    
    def getDataById(self, id: Any) -> DAData:
        """
        根据 ID 获取数据
        
        Args:
            id: 数据标识符
            
        Returns:
            匹配的 DAData 对象
        """
        ...
    
    def getAllDatas(self) -> List[DAData]:
        """
        获取所有数据
        
        Returns:
            数据管理器中的所有 DAData 对象列表
        """
        ...
    
    def getAllDataframes(self) -> Dict[str, pd.DataFrame]:
        """
        获取所有数据框
        
        Returns:
            字典，键为数据名称，值为对应的 pandas DataFrame
        """
        ...
    
    def getSelectDatas(self) -> List[DAData]:
        """
        获取选中的数据
        
        Returns:
            当前选中的 DAData 对象列表
        """
        ...
    
    def getOperateData(self) -> DAData:
        """
        获取当前操作数据
        
        Returns:
            当前正在操作的 DAData 对象
        """
        ...
    
    def getOperateDataSeries(self) -> List[int]:
        """
        获取当前操作数据系列
        
        Returns:
            当前操作数据的列索引列表
        """
        ...
    
    def getSelectDataframes(self) -> Dict[str, pd.DataFrame]:
        """
        获取选中的数据框
        
        Returns:
            字典，键为选中数据名称，值为对应的 pandas DataFrame
        """
        ...
    
    def findDatas(self, pattern: str, cs: int = 0) -> List[DAData]:
        """
        查找数据
        
        Args:
            pattern: 搜索模式字符串
            cs: 大小写敏感选项，0=不区分大小写，1=区分大小写
            
        Returns:
            匹配搜索模式的 DAData 对象列表
        """
        ...
    
    def findDatasReg(self, regex_pattern: str) -> List[DAData]:
        """
        使用正则表达式查找数据
        
        Args:
            regex_pattern: 正则表达式模式
            
        Returns:
            匹配正则表达式的 DAData 对象列表
        """
        ...
    
    def addDataframe(self, df: pd.DataFrame, name: str) -> None:
        """
        添加数据框
        
        快捷方法：将 pandas DataFrame 转换为 DAData 并添加到管理器。
        
        Args:
            df: pandas DataFrame 对象
            name: 数据名称
        """
        ...
    
    def addSeries(self, series: pd.Series, name: str) -> None:
        """
        添加数据系列
        
        快捷方法：将 pandas Series 转换为 DAData 并添加到管理器。
        
        Args:
            series: pandas Series 对象
            name: 数据名称
        """
        ...


class DAStatusBarInterface:
    """
    状态栏接口
    
    提供对应用程序状态栏的控制功能。
    """
    
    def showMessage(self, message: str, timeout: int = 15000) -> None:
        """
        显示消息
        
        Args:
            message: 要显示的消息文本
            timeout: 消息显示时间（毫秒），默认 15000 毫秒
        """
        ...
    
    def clearMessage(self) -> None:
        """
        清除消息
        
        清除状态栏上显示的消息。
        """
        ...
    
    def showProgressBar(self) -> None:
        """
        显示进度条
        
        在状态栏中显示进度条。
        """
        ...
    
    def hideProgressBar(self) -> None:
        """
        隐藏进度条
        
        隐藏状态栏中的进度条。
        """
        ...
    
    def setProgress(self, value: int) -> None:
        """
        设置进度值
        
        Args:
            value: 进度值（0-100）
        """
        ...
    
    def setProgressText(self, text: str) -> None:
        """
        设置进度文本
        
        Args:
            text: 进度条显示的文本
        """
        ...
    
    def clearProgressText(self) -> None:
        """
        清除进度文本
        
        清除进度条上显示的文本。
        """
        ...
    
    def setBusy(self, busy: bool) -> None:
        """
        设置忙碌状态
        
        Args:
            busy: True 表示忙碌状态，False 表示空闲状态
        """
        ...
    
    def isBusy(self) -> bool:
        """
        检查是否忙碌
        
        Returns:
            True 如果处于忙碌状态，否则 False
        """
        ...
    
    def resetProgress(self) -> None:
        """
        重置进度
        
        重置进度条到初始状态。
        """
        ...
    
    def isProgressBarVisible(self) -> bool:
        """
        检查进度条是否可见
        
        Returns:
            True 如果进度条可见，否则 False
        """
        ...


class DACommandInterface:
    """
    命令接口
    
    提供命令执行和撤销/重做功能。
    """
    
    def ui(self) -> "DAUIInterface":
        """
        获取 UI 接口
        
        Returns:
            DAUIInterface 对象引用
        """
        ...
    
    def beginDataOperateCommand(self, data: DAData, text: str, isObjectPersist: bool = True, isSkipFirstRedo: bool = True) -> None:
        """
        开始数据操作命令
        
        开始一个数据操作命令，支持撤销/重做。
        
        Args:
            data: 要操作的数据对象
            text: 命令描述文本
            isObjectPersist: 是否持久化对象，默认 True
            isSkipFirstRedo: 是否跳过第一次重做，默认 True
        """
        ...
    
    def endDataOperateCommand(self, data: DAData) -> None:
        """
        结束数据操作命令
        
        结束一个数据操作命令，与 beginDataOperateCommand 配对使用。
        
        Args:
            data: 操作完成的数据对象
        """
        ...


class DAUIInterface:
    """
    UI 接口
    
    提供对应用程序用户界面的控制功能。
    """
    
    def getStatusBar(self) -> DAStatusBarInterface:
        """
        获取状态栏接口
        
        Returns:
            DAStatusBarInterface 对象引用
        """
        ...
    
    def processEvents(self) -> None:
        """
        处理事件
        
        处理 Qt 事件队列中的所有待处理事件。
        """
        ...
    
    def addInfoLogMessage(self, msg: str, showInStatusBar: bool = True) -> None:
        """
        添加信息日志消息
        
        Args:
            msg: 日志消息文本
            showInStatusBar: 是否在状态栏显示，默认 True
        """
        ...
    
    def addWarningLogMessage(self, msg: str, showInStatusBar: bool = True) -> None:
        """
        添加警告日志消息
        
        Args:
            msg: 日志消息文本
            showInStatusBar: 是否在状态栏显示，默认 True
        """
        ...
    
    def addCriticalLogMessage(self, msg: str, showInStatusBar: bool = True) -> None:
        """
        添加严重日志消息
        
        Args:
            msg: 日志消息文本
            showInStatusBar: 是否在状态栏显示，默认 True
        """
        ...
    
    def getCommandInterface(self) -> DACommandInterface:
        """
        获取命令接口
        
        Returns:
            DACommandInterface 对象引用
        """
        ...
    
    def getConfigValues(self, jsonConfig: str, cacheKey: str = "") -> Dict[str, Any]:
        """
        获取配置值
        
        通过 JSON 配置对话框获取用户配置值。
        
        Args:
            jsonConfig: JSON 格式的配置定义
            cacheKey: 缓存键，用于记住上次的选择
            
        Returns:
            字典，包含用户选择的配置值
        """
        ...
    
    def getExistingDirectory(self, title: str = "", dir: str = "") -> str:
        """
        获取现有目录
        
        打开目录选择对话框。
        
        Args:
            title: 对话框标题
            dir: 初始目录路径
            
        Returns:
            用户选择的目录路径
        """
        ...
    
    def setDirty(self, on: bool = True) -> None:
        """
        设置脏标志
        
        标记项目为已修改（脏）状态。
        
        Args:
            on: True 设置脏标志，False 清除脏标志，默认 True
        """
        ...


class DACoreInterface:
    """
    核心接口
    
    提供对应用程序核心功能的访问。
    """
    
    def getUiInterface(self) -> DAUIInterface:
        """
        获取 UI 接口
        
        Returns:
            DAUIInterface 对象引用
        """
        ...
    
    def getDataManagerInterface(self) -> DADataManagerInterface:
        """
        获取数据管理器接口
        
        Returns:
            DADataManagerInterface 对象引用
        """
        ...
    
    def getPythonSignalHandler(self) -> DAPythonSignalHandler:
        """
        获取 Python 信号处理器
        
        Returns:
            DAPythonSignalHandler 对象引用
        """
        ...
    
    def isProjectDirty(self) -> bool:
        """
        检查项目是否已修改
        
        Returns:
            True 如果项目有未保存的修改，否则 False
        """
        ...
    
    def setProjectDirty(self, on: bool) -> None:
        """
        设置项目修改状态
        
        Args:
            on: True 标记项目为已修改，False 标记为未修改
        """
        ...