"""
da_interface 模块的 Mock 实现

此模块提供 Qt 应用程序核心接口的 Mock 实现，包括数据管理、UI 操作、命令执行和线程调度等功能。
此实现工作在标准 Python 环境中（无需 Qt 或 C++ 程序）。
"""

from typing import Any, Callable, Dict, List, Optional, Union
import pandas as pd
import re

# 使用懒加载导入 da_data 模块以避免循环依赖
_da_data_module = None

def _get_da_data_module():
    """懒加载 da_data 模块"""
    global _da_data_module
    if _da_data_module is None:
        # 尝试从当前目录导入
        try:
            from . import da_data
            _da_data_module = da_data
        except ImportError:
            # 如果失败，尝试从 stubs.mock 导入
            try:
                from stubs.mock import da_data
                _da_data_module = da_data
            except ImportError:
                # 最后尝试直接导入
                import da_data
                _da_data_module = da_data
    return _da_data_module

def _get_DAData():
    """获取 DAData 类"""
    return _get_da_data_module().DAData

def _get_DADataManager():
    """获取 DADataManager 类"""
    return _get_da_data_module().DADataManager

def _get_DataChangeType():
    """获取 DataChangeType 枚举"""
    return _get_da_data_module().DataChangeType


class DAPythonSignalHandler:
    """
    Python 信号处理器
    
    此类用于在 Qt 主线程中调度 Python 函数的执行。
    在 Mock 实现中，直接调用函数而不进行线程调度。
    """
    
    def callInMainThread(self, func: Callable) -> None:
        """
        在 Qt 主线程中调度 Python 函数
        
        此方法将 Python 函数调度到 Qt 主线程中执行，确保线程安全。
        在 Mock 实现中，直接调用函数而不进行线程调度。
        
        Args:
            func: 要执行的 Python 可调用对象
        """
        print(f"[Mock DAPythonSignalHandler] 调用函数: {func.__name__ if hasattr(func, '__name__') else func}")
        func()


class DADataManagerInterface:
    """
    数据管理器接口
    
    提供对应用程序数据管理器的访问，支持数据的增删改查操作。
    在 Mock 实现中，内部使用 DADataManager 实例。
    """
    
    def __init__(self):
        """初始化数据管理器接口"""
        self._mgr = _get_DADataManager()()
        print("[Mock DADataManagerInterface] 初始化")
    
    def addData(self, data: Any) -> None:
        """
        立即添加数据
        
        将数据添加到数据管理器中，不记录撤销/重做操作。
        
        Args:
            data: 要添加的 DAData 对象
        """
        print(f"[Mock DADataManagerInterface] 添加数据: {data.getName() if hasattr(data, 'getName') else data}")
        self._mgr.addData(data)
    
    def addData_(self, data: Any) -> None:
        """
        添加数据（支持撤销/重做）
        
        将数据添加到数据管理器中，并记录撤销/重做操作。
        
        Args:
            data: 要添加的 DAData 对象
        """
        print(f"[Mock DADataManagerInterface] 添加数据（支持撤销/重做）: {data.getName() if hasattr(data, 'getName') else data}")
        self._mgr.addData_(data)
    
    def removeData(self, data: Any) -> None:
        """
        立即删除数据
        
        从数据管理器中删除数据，不记录撤销/重做操作。
        
        Args:
            data: 要删除的 DAData 对象
        """
        print(f"[Mock DADataManagerInterface] 删除数据: {data.getName() if hasattr(data, 'getName') else data}")
        self._mgr.removeData(data)
    
    def removeData_(self, data: Any) -> None:
        """
        删除数据（支持撤销/重做）
        
        从数据管理器中删除数据，并记录撤销/重做操作。
        
        Args:
            data: 要删除的 DAData 对象
        """
        print(f"[Mock DADataManagerInterface] 删除数据（支持撤销/重做）: {data.getName() if hasattr(data, 'getName') else data}")
        self._mgr.removeData_(data)
    
    def getDataCount(self) -> int:
        """
        获取数据数量
        
        Returns:
            数据管理器中的数据总数
        """
        count = self._mgr.getDataCount()
        print(f"[Mock DADataManagerInterface] 获取数据数量: {count}")
        return count
    
    def getData(self, index: int) -> Any:
        """
        根据索引获取数据
        
        Args:
            index: 数据索引
            
        Returns:
            指定索引处的 DAData 对象
        """
        data = self._mgr.getData(index)
        print(f"[Mock DADataManagerInterface] 根据索引获取数据: index={index}, name={data.getName() if hasattr(data, 'getName') else 'Unknown'}")
        return data
    
    def getDataIndex(self, data: Any) -> int:
        """
        获取数据索引
        
        Args:
            data: DAData 对象
            
        Returns:
            数据在管理器中的索引，如果未找到则返回 -1
        """
        index = self._mgr.getDataIndex(data)
        print(f"[Mock DADataManagerInterface] 获取数据索引: name={data.getName() if hasattr(data, 'getName') else 'Unknown'}, index={index}")
        return index
    
    def getDataById(self, id: Any) -> Any:
        """
        根据 ID 获取数据
        
        Args:
            id: 数据标识符
            
        Returns:
            匹配的 DAData 对象
        """
        # 在 Mock 实现中，遍历所有数据查找匹配的 ID
        all_datas = self.getAllDatas()
        for data in all_datas:
            if hasattr(data, 'id') and data.id() == id:
                print(f"[Mock DADataManagerInterface] 根据 ID 获取数据: id={id}, name={data.getName() if hasattr(data, 'getName') else 'Unknown'}")
                return data
        print(f"[Mock DADataManagerInterface] 根据 ID 获取数据: id={id}, 未找到")
        return None
    
    def getAllDatas(self) -> List[Any]:
        """
        获取所有数据
        
        Returns:
            数据管理器中的所有 DAData 对象列表
        """
        datas = self._mgr.getAllDatas()
        print(f"[Mock DADataManagerInterface] 获取所有数据: 共 {len(datas)} 个")
        return datas
    
    def getAllDataframes(self) -> Dict[str, pd.DataFrame]:
        """
        获取所有数据框
        
        Returns:
            字典，键为数据名称，值为对应的 pandas DataFrame
        """
        all_datas = self.getAllDatas()
        result = {}
        for data in all_datas:
            if hasattr(data, 'isDataFrame') and data.isDataFrame():
                name = data.getName() if hasattr(data, 'getName') else f"Data_{id(data)}"
                try:
                    df = data.toDataFrame()
                    result[name] = df
                except:
                    pass
        print(f"[Mock DADataManagerInterface] 获取所有数据框: 共 {len(result)} 个")
        return result
    
    def getSelectDatas(self) -> List[Any]:
        """
        获取选中的数据
        
        Returns:
            当前选中的 DAData 对象列表
        """
        # 在 Mock 实现中，返回所有数据（因为没有真正的选择状态）
        datas = self.getAllDatas()
        print(f"[Mock DADataManagerInterface] 获取选中的数据: 返回所有 {len(datas)} 个数据")
        return datas
    
    def getOperateData(self) -> Any:
        """
        获取当前操作数据
        
        Returns:
            当前正在操作的 DAData 对象
        """
        # 在 Mock 实现中，返回第一个数据或 None
        all_datas = self.getAllDatas()
        if all_datas:
            data = all_datas[0]
            print(f"[Mock DADataManagerInterface] 获取当前操作数据: {data.getName() if hasattr(data, 'getName') else 'Unknown'}")
            return data
        print("[Mock DADataManagerInterface] 获取当前操作数据: 无数据")
        return None
    
    def getOperateDataSeries(self) -> List[int]:
        """
        获取当前操作数据系列
        
        Returns:
            当前操作数据的列索引列表
        """
        # 在 Mock 实现中，返回空列表
        print("[Mock DADataManagerInterface] 获取当前操作数据系列: 返回空列表")
        return []
    
    def getSelectDataframes(self) -> Dict[str, pd.DataFrame]:
        """
        获取选中的数据框
        
        Returns:
            字典，键为选中数据名称，值为对应的 pandas DataFrame
        """
        # 在 Mock 实现中，返回所有数据框（因为没有真正的选择状态）
        result = self.getAllDataframes()
        print(f"[Mock DADataManagerInterface] 获取选中的数据框: 共 {len(result)} 个")
        return result
    
    def findDatas(self, pattern: str, cs: int = 0) -> List[Any]:
        """
        查找数据
        
        Args:
            pattern: 搜索模式字符串
            cs: 大小写敏感选项，0=不区分大小写，1=区分大小写
            
        Returns:
            匹配搜索模式的 DAData 对象列表
        """
        # 委托给内部管理器
        result = self._mgr.findDatas(pattern, cs)
        print(f"[Mock DADataManagerInterface] 查找数据: pattern='{pattern}', cs={cs}, 找到 {len(result)} 个")
        return result
    
    def findDatasReg(self, regex_pattern: str) -> List[Any]:
        """
        使用正则表达式查找数据
        
        Args:
            regex_pattern: 正则表达式模式
            
        Returns:
            匹配正则表达式的 DAData 对象列表
        """
        # 委托给内部管理器
        result = self._mgr.findDatasReg(regex_pattern)
        print(f"[Mock DADataManagerInterface] 使用正则表达式查找数据: pattern='{regex_pattern}', 找到 {len(result)} 个")
        return result
    
    def addDataframe(self, df: pd.DataFrame, name: str) -> None:
        """
        添加数据框
        
        快捷方法：将 pandas DataFrame 转换为 DAData 并添加到管理器。
        
        Args:
            df: pandas DataFrame 对象
            name: 数据名称
        """
        print(f"[Mock DADataManagerInterface] 添加数据框: name='{name}', shape={df.shape}")
        self._mgr.addDataFrame(df, name)
    
    def addSeries(self, series: pd.Series, name: str) -> None:
        """
        添加数据系列
        
        快捷方法：将 pandas Series 转换为 DAData 并添加到管理器。
        
        Args:
            series: pandas Series 对象
            name: 数据名称
        """
        print(f"[Mock DADataManagerInterface] 添加数据系列: name='{name}', length={len(series)}")
        # Create DAData from series and add it
        DAData = _get_DAData()
        data = DAData(series)
        data.setName(name)
        self._mgr.addData(data)


class DAStatusBarInterface:
    """
    状态栏接口
    
    提供对应用程序状态栏的控制功能。
    在 Mock 实现中，所有操作打印日志但不执行实际 UI 操作。
    """
    
    def showMessage(self, message: str, timeout: int = 15000) -> None:
        """
        显示消息
        
        Args:
            message: 要显示的消息文本
            timeout: 消息显示时间（毫秒），默认 15000 毫秒
        """
        print(f"[Mock DAStatusBarInterface] 显示消息: '{message}', timeout={timeout}ms")
    
    def clearMessage(self) -> None:
        """
        清除消息
        
        清除状态栏上显示的消息。
        """
        print("[Mock DAStatusBarInterface] 清除消息")
    
    def showProgressBar(self) -> None:
        """
        显示进度条
        
        在状态栏中显示进度条。
        """
        print("[Mock DAStatusBarInterface] 显示进度条")
    
    def hideProgressBar(self) -> None:
        """
        隐藏进度条
        
        隐藏状态栏中的进度条。
        """
        print("[Mock DAStatusBarInterface] 隐藏进度条")
    
    def setProgress(self, value: int) -> None:
        """
        设置进度值
        
        Args:
            value: 进度值（0-100）
        """
        print(f"[Mock DAStatusBarInterface] 设置进度值: {value}")
    
    def setProgressText(self, text: str) -> None:
        """
        设置进度文本
        
        Args:
            text: 进度条显示的文本
        """
        print(f"[Mock DAStatusBarInterface] 设置进度文本: '{text}'")
    
    def clearProgressText(self) -> None:
        """
        清除进度文本
        
        清除进度条上显示的文本。
        """
        print("[Mock DAStatusBarInterface] 清除进度文本")
    
    def setBusy(self, busy: bool) -> None:
        """
        设置忙碌状态
        
        Args:
            busy: True 表示忙碌状态，False 表示空闲状态
        """
        print(f"[Mock DAStatusBarInterface] 设置忙碌状态: {busy}")
    
    def isBusy(self) -> bool:
        """
        检查是否忙碌
        
        Returns:
            True 如果处于忙碌状态，否则 False
        """
        print("[Mock DAStatusBarInterface] 检查是否忙碌: 返回 False")
        return False
    
    def resetProgress(self) -> None:
        """
        重置进度
        
        重置进度条到初始状态。
        """
        print("[Mock DAStatusBarInterface] 重置进度")
    
    def isProgressBarVisible(self) -> bool:
        """
        检查进度条是否可见
        
        Returns:
            True 如果进度条可见，否则 False
        """
        print("[Mock DAStatusBarInterface] 检查进度条是否可见: 返回 False")
        return False


class DACommandInterface:
    """
    命令接口
    
    提供命令执行和撤销/重做功能。
    在 Mock 实现中，所有操作打印日志但不执行实际命令。
    """
    
    def __init__(self, ui_interface: Any = None):
        """初始化命令接口"""
        self._ui = ui_interface
        print("[Mock DACommandInterface] 初始化")
    
    def ui(self) -> Any:
        """
        获取 UI 接口
        
        Returns:
            DAUIInterface 对象引用
        """
        print("[Mock DACommandInterface] 获取 UI 接口")
        return self._ui
    
    def beginDataOperateCommand(self, data: Any, text: str, isObjectPersist: bool = True, isSkipFirstRedo: bool = True) -> None:
        """
        开始数据操作命令
        
        开始一个数据操作命令，支持撤销/重做。
        
        Args:
            data: 要操作的数据对象
            text: 命令描述文本
            isObjectPersist: 是否持久化对象，默认 True
            isSkipFirstRedo: 是否跳过第一次重做，默认 True
        """
        data_name = data.getName() if hasattr(data, 'getName') else 'Unknown'
        print(f"[Mock DACommandInterface] 开始数据操作命令: data='{data_name}', text='{text}', "
              f"isObjectPersist={isObjectPersist}, isSkipFirstRedo={isSkipFirstRedo}")
    
    def endDataOperateCommand(self, data: Any) -> None:
        """
        结束数据操作命令
        
        结束一个数据操作命令，与 beginDataOperateCommand 配对使用。
        
        Args:
            data: 操作完成的数据对象
        """
        data_name = data.getName() if hasattr(data, 'getName') else 'Unknown'
        print(f"[Mock DACommandInterface] 结束数据操作命令: data='{data_name}'")


class DAUIInterface:
    """
    UI 接口
    
    提供对应用程序用户界面的控制功能。
    在 Mock 实现中，所有操作打印日志但不执行实际 UI 操作。
    """
    
    def __init__(self):
        """初始化 UI 接口"""
        self._status_bar = DAStatusBarInterface()
        self._command_interface = DACommandInterface(self)
        print("[Mock DAUIInterface] 初始化")
    
    def getStatusBar(self) -> DAStatusBarInterface:
        """
        获取状态栏接口
        
        Returns:
            DAStatusBarInterface 对象引用
        """
        print("[Mock DAUIInterface] 获取状态栏接口")
        return self._status_bar
    
    def processEvents(self) -> None:
        """
        处理事件
        
        处理 Qt 事件队列中的所有待处理事件。
        """
        print("[Mock DAUIInterface] 处理事件")
    
    def addInfoLogMessage(self, msg: str, showInStatusBar: bool = True) -> None:
        """
        添加信息日志消息
        
        Args:
            msg: 日志消息文本
            showInStatusBar: 是否在状态栏显示，默认 True
        """
        print(f"[INFO] {msg}")
        if showInStatusBar:
            self._status_bar.showMessage(msg)
    
    def addWarningLogMessage(self, msg: str, showInStatusBar: bool = True) -> None:
        """
        添加警告日志消息
        
        Args:
            msg: 日志消息文本
            showInStatusBar: 是否在状态栏显示，默认 True
        """
        print(f"[WARNING] {msg}")
        if showInStatusBar:
            self._status_bar.showMessage(msg)
    
    def addCriticalLogMessage(self, msg: str, showInStatusBar: bool = True) -> None:
        """
        添加严重日志消息
        
        Args:
            msg: 日志消息文本
            showInStatusBar: 是否在状态栏显示，默认 True
        """
        print(f"[CRITICAL] {msg}")
        if showInStatusBar:
            self._status_bar.showMessage(msg)
    
    def getCommandInterface(self) -> DACommandInterface:
        """
        获取命令接口
        
        Returns:
            DACommandInterface 对象引用
        """
        print("[Mock DAUIInterface] 获取命令接口")
        return self._command_interface
    
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
        print(f"[Mock DAUIInterface] 获取配置值: jsonConfig长度={len(jsonConfig)}, cacheKey='{cacheKey}'")
        # 返回空字典作为 Mock
        return {}
    
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
        print(f"[Mock DAUIInterface] 获取现有目录: title='{title}', dir='{dir}'")
        # 返回空字符串作为 Mock
        return ""
    
    def setDirty(self, on: bool = True) -> None:
        """
        设置脏标志
        
        标记项目为已修改（脏）状态。
        
        Args:
            on: True 设置脏标志，False 清除脏标志，默认 True
        """
        print(f"[Mock DAUIInterface] 设置脏标志: {on}")


class DACoreInterface:
    """
    核心接口
    
    提供对应用程序核心功能的访问。
    在 Mock 实现中，组合所有 Mock 子接口。
    """
    
    def __init__(self):
        """初始化核心接口"""
        self._ui_interface = DAUIInterface()
        self._data_manager_interface = DADataManagerInterface()
        self._python_signal_handler = DAPythonSignalHandler()
        self._project_dirty = False
        print("[Mock DACoreInterface] 初始化")
    
    def getUiInterface(self) -> DAUIInterface:
        """
        获取 UI 接口
        
        Returns:
            DAUIInterface 对象引用
        """
        print("[Mock DACoreInterface] 获取 UI 接口")
        return self._ui_interface
    
    def getDataManagerInterface(self) -> DADataManagerInterface:
        """
        获取数据管理器接口
        
        Returns:
            DADataManagerInterface 对象引用
        """
        print("[Mock DACoreInterface] 获取数据管理器接口")
        return self._data_manager_interface
    
    def getPythonSignalHandler(self) -> DAPythonSignalHandler:
        """
        获取 Python 信号处理器
        
        Returns:
            DAPythonSignalHandler 对象引用
        """
        print("[Mock DACoreInterface] 获取 Python 信号处理器")
        return self._python_signal_handler
    
    def isProjectDirty(self) -> bool:
        """
        检查项目是否已修改
        
        Returns:
            True 如果项目有未保存的修改，否则 False
        """
        print(f"[Mock DACoreInterface] 检查项目是否已修改: {self._project_dirty}")
        return self._project_dirty
    
    def setProjectDirty(self, on: bool) -> None:
        """
        设置项目修改状态
        
        Args:
            on: True 标记项目为已修改，False 标记为未修改
        """
        print(f"[Mock DACoreInterface] 设置项目修改状态: {on}")
        self._project_dirty = on