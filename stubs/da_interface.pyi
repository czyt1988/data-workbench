from typing import Any, List, Dict, Callable, Optional, Union
import pandas as pd

class DAPythonSignalHandler:
    """
    Python signal handler for cross-thread communication
    """
    def callInMainThread(self, func: Callable) -> None:
        """
        Schedule a Python function to be executed in the Qt main thread
        
        Args:
            func: Python function to execute in main thread
        """
        ...

class DAData:
    """
    Data wrapper for pandas DataFrame/Series
    """
    def __init__(self) -> None: ...
    def __init__(self, obj: Any) -> None: ...
    
    def toPyObject(self) -> Any:
        """
        Return the underlying pandas DataFrame as a Python object
        """
        ...
    
    def toDataFrame(self) -> pd.DataFrame:
        """
        Convert to DataFrame
        """
        ...
    
    def toSeries(self) -> pd.Series:
        """
        Convert to Series
        """
        ...
    
    def getName(self) -> str:
        """
        Get data name
        """
        ...
    
    def setName(self, name: str) -> None:
        """
        Set data name
        """
        ...
    
    def getDescribe(self) -> str:
        """
        Get data description
        """
        ...
    
    def setDescribe(self, desc: str) -> None:
        """
        Set data description
        """
        ...
    
    def isNull(self) -> bool:
        """
        Check if the data is null
        """
        ...
    
    def id(self) -> Any:
        """
        Return the data id
        """
        ...
    
    def isDataFrame(self) -> bool:
        """
        Check if the data type is DataFrame
        """
        ...
    
    def isSeries(self) -> bool:
        """
        Check if the data type is Series
        """
        ...
    
    def getDataManager(self) -> Any:
        """
        Get the DataManager
        """
        ...

class DADataManagerInterface:
    """
    Interface for data management operations
    """
    def addData(self, data: DAData) -> None:
        """
        Add data immediately
        """
        ...
    
    def addData_(self, data: DAData) -> None:
        """
        Add data with undo/redo support
        """
        ...
    
    def removeData(self, data: DAData) -> None:
        """
        Remove data without undo/redo
        """
        ...
    
    def removeData_(self, data: DAData) -> None:
        """
        Remove data with undo/redo support
        """
        ...
    
    def getDataCount(self) -> int:
        """
        Get total number of data items
        """
        ...
    
    def getData(self, index: int) -> DAData:
        """
        Get data by index
        """
        ...
    
    def getDataIndex(self, data: DAData) -> int:
        """
        Get index of data in manager
        """
        ...
    
    def getDataById(self, id: Any) -> DAData:
        """
        Get data by ID
        """
        ...
    
    def getAllDatas(self) -> List[DAData]:
        """
        Get all data objects as a list
        """
        ...
    
    def getAllDataframes(self) -> Dict[str, pd.DataFrame]:
        """
        Get all dataframe objects as a dict {dataname: dataframe}
        """
        ...
    
    def getSelectDatas(self) -> List[DAData]:
        """
        Get selected data objects as a list
        """
        ...
    
    def getSelectDataframes(self) -> Dict[str, pd.DataFrame]:
        """
        Get selected dataframe objects as a dict {dataname: dataframe}
        """
        ...
    
    def findDatas(self, pattern: str, cs: int = 0) -> List[DAData]:
        """
        Find datas by name pattern
        
        Args:
            pattern: Name pattern to search
            cs: Case sensitivity (0=CaseInsensitive, 1=CaseSensitive)
        """
        ...
    
    def findDatasReg(self, regex_pattern: str) -> List[DAData]:
        """
        Find datas by regular expression
        """
        ...
    
    def addDataframe(self, df: pd.DataFrame, name: str) -> None:
        """
        Add pandas DataFrame to data manager
        """
        ...
    
    def addSeries(self, series: pd.Series, name: str) -> None:
        """
        Add pandas Series to data manager
        """
        ...

class DAStatusBarInterface:
    """
    Interface for status bar operations
    """
    def showMessage(self, message: str, timeout: int = 15000) -> None:
        """
        Show message in status bar
        """
        ...
    
    def clearMessage(self) -> None:
        """
        Clear status bar message
        """
        ...
    
    def showProgressBar(self) -> None:
        """
        Show progress bar
        """
        ...
    
    def hideProgressBar(self) -> None:
        """
        Hide progress bar
        """
        ...
    
    def setProgress(self, value: int) -> None:
        """
        Set progress bar value
        """
        ...
    
    def setProgressText(self, text: str) -> None:
        """
        Set progress bar text
        """
        ...
    
    def clearProgressText(self) -> None:
        """
        Clear progress bar text
        """
        ...
    
    def setBusy(self, busy: bool) -> None:
        """
        Set busy indicator
        """
        ...
    
    def isBusy(self) -> bool:
        """
        Check if busy indicator is active
        """
        ...
    
    def resetProgress(self) -> None:
        """
        Reset progress bar
        """
        ...
    
    def isProgressBarVisible(self) -> bool:
        """
        Check if progress bar is visible
        """
        ...

class DAUIInterface:
    """
    Interface for UI operations
    """
    def getStatusBar(self) -> DAStatusBarInterface:
        """
        Get status bar interface
        """
        ...
    
    def processEvents(self) -> None:
        """
        Process pending events
        """
        ...
    
    def addInfoLogMessage(self, msg: str, showInStatusBar: bool = True) -> None:
        """
        Add info message to log
        """
        ...
    
    def addWarningLogMessage(self, msg: str, showInStatusBar: bool = True) -> None:
        """
        Add warning message to log
        """
        ...
    
    def addCriticalLogMessage(self, msg: str, showInStatusBar: bool = True) -> None:
        """
        Add critical message to log
        """
        ...

class DACoreInterface:
    """
    Core application interface
    """
    def getUiInterface(self) -> DAUIInterface:
        """
        Get UI interface
        """
        ...
    
    def getDataManagerInterface(self) -> DADataManagerInterface:
        """
        Get data manager interface
        """
        ...
    
    def getPythonSignalHandler(self) -> DAPythonSignalHandler:
        """
        Get Python signal handler for cross-thread communication
        """
        ...
    
    def isProjectDirty(self) -> bool:
        """
        Check if project has unsaved changes
        """
        ...
    
    def setProjectDirty(self, on: bool) -> None:
        """
        Set project dirty flag
        """
        ...