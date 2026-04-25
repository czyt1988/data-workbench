import threading
import time
import uuid
from typing import Dict, Optional, List, Any, Tuple
from datetime import datetime
from collections import OrderedDict

class ProcessingStatus:
    """通用的线程任务状态管理器"""
    
    def __init__(self, task_id: str):
        """
        初始化状态管理器
        
        Args:
            task_id: 任务唯一标识（字符串类型，由系统生成）
        """
        self._task_id = task_id  # 字符串类型的task_id
        self._lock = threading.RLock()  # 可重入锁，支持嵌套调用
        self._is_processing = False     # 是否正在处理
        self._is_paused = False         # 是否暂停
        self._is_canceled = False       # 是否取消
        self._is_success = False        # 是否成功完成
        self._start_time = None         # 开始时间
        self._end_time = None           # 结束时间
        self._progress = 0.0            # 当前进度（0~100）
        self._current_stage = ""        # 当前阶段描述
        self._message = ""              # 附加消息（如错误信息）
        self._custom_data = {}          # 自定义数据（键值对，灵活扩展）
        self._task_name = ""            # 可选的任务名称

    @property
    def task_id(self) -> str:
        """获取任务唯一标识（字符串）"""
        return self._task_id

    def start(self, task_name: str = ""):
        """启动任务"""
        with self._lock:
            self._is_processing = True
            self._is_paused = False
            self._is_canceled = False
            self._is_success = False
            self._start_time = time.time()
            self._end_time = None
            self._progress = 0.0
            self._current_stage = "初始化"
            self._message = ""
            self._task_name = task_name
            self._custom_data.clear()

    def pause(self):
        """暂停任务"""
        with self._lock:
            if self._is_processing and not self._is_paused:
                self._is_paused = True
                self._current_stage = "已暂停"

    def resume(self):
        """恢复任务"""
        with self._lock:
            if self._is_paused:
                self._is_paused = False
                self._current_stage = "继续处理"

    def cancel(self, message: str = "任务已取消"):
        """取消任务"""
        with self._lock:
            if self._is_processing and not self._is_canceled:
                self._is_processing = False
                self._is_canceled = True
                self._end_time = time.time()
                self._progress = 0.0  # 取消后进度重置为0
                self._current_stage = "已取消"
                self._message = message

    def finish(self, success: bool = True, message: str = ""):
        """完成任务"""
        with self._lock:
            if self._is_processing and not self._is_canceled:
                self._is_processing = False
                self._is_success = success
                self._end_time = time.time()
                self._progress = 100.0 if success else self._progress  # 成功则进度100%
                self._current_stage = "完成" if success else "失败"
                self._message = message

    def update_progress(self, progress: float, message: Optional[str] = None):
        """
        更新进度
        
        Args:
            progress: 进度值（0~100），超出范围会被截断
            stage: 当前阶段描述（可选）
            message: 附加消息（可选）
        """
        with self._lock:
            self._progress = max(0.0, min(100.0, progress))  # 限制进度在0~100
            if message:
                self._message = message

    def update_custom_data(self, key: str, value: Any):
        """更新自定义数据（存储任务特定信息）"""
        with self._lock:
            self._custom_data[key] = value

    def get_custom_data(self, key: str, default: Any = None) -> Any:
        """获取自定义数据"""
        with self._lock:
            return self._custom_data.get(key, default)

    def get_status(self) -> Dict[str, Any]:
        """获取当前任务状态（供界面展示）"""
        with self._lock:
            elapsed = None
            if self._start_time:
                end = self._end_time or time.time()
                elapsed = round(end - self._start_time, 2)
            
            return {
                "task_id": self._task_id,
                "task_name": self._task_name,
                "is_running": self._is_processing and not self._is_paused and not self._is_canceled,
                "is_paused": self._is_paused,
                "is_canceled": self._is_canceled,
                "is_success": self._is_success,
                "current_stage": self._current_stage,
                "progress": round(self._progress, 2),
                "elapsed_seconds": elapsed,
                "message": self._message,
                "custom_data": self._custom_data.copy()  # 自定义数据副本
            }

    def is_active(self) -> bool:
        """判断任务是否处于活跃状态（运行中或暂停中）"""
        with self._lock:
            return self._is_processing and not self._is_canceled

    def is_finished(self) -> bool:
        """判断任务是否已完成（成功、失败或取消）"""
        with self._lock:
            return not self._is_processing or self._is_canceled

"""
# 使用示例函数
def create_and_run_example_task(task_name: str) -> str:
    # 创建新任务状态
    task_id, status = global_status_manager.create_status(task_name)
    
    # 在后台线程中运行任务
    def run_task():
        try:
            # 模拟任务执行过程
            for i in range(1, 11):
                time.sleep(0.5)  # 模拟耗时操作
                progress = i * 10
                status.update_progress(progress, f"正在处理第{i}步...")
                
                # 可以随时检查是否被取消
                if status._is_canceled:
                    return
            
            # 任务完成
            status.finish(True, "任务执行成功")
        except Exception as e:
            # 任务失败
            status.finish(False, f"任务执行失败: {str(e)}")
    
    # 启动任务线程
    thread = threading.Thread(target=run_task, daemon=True)
    thread.start()
    
    return task_id
"""
class StatusManager:
    """任务状态管理器（自动生成任务ID）"""
    
    def __init__(self, max_history: int = 100):
        """
        初始化状态管理器
        
        Args:
            max_history: 最大历史任务数量，超过此数量会自动清理最早的任务
        """
        self._lock = threading.Lock()
        self._status_instances: OrderedDict[str, ProcessingStatus] = OrderedDict()
        self._max_history = max_history
        
    def _generate_task_id(self) -> str:
        """生成唯一的任务ID"""
        return str(uuid.uuid4())

    def create_status(self, task_name: str = "") -> Tuple[str, ProcessingStatus]:
        """
        创建新任务状态实例并自动生成任务ID
        
        Args:
            task_name: 可选的任务名称，用于描述性显示
            
        Returns:
            Tuple[str, ProcessingStatus]: (task_id, status_instance)
        """
        with self._lock:
            # 生成唯一任务ID
            task_id = self._generate_task_id()
            
            # 创建状态实例
            status = ProcessingStatus(task_id)
            status.start(task_name)
            
            # 添加到管理器中
            self._status_instances[task_id] = status
            
            # 清理历史记录，保持不超过最大数量
            self._cleanup_old_tasks()
            
            return task_id, status

    def get_status(self, task_id: str) -> Optional[ProcessingStatus]:
        """获取指定task_id的状态实例"""
        with self._lock:
            return self._status_instances.get(task_id)

    def get_status_info(self, task_id: str) -> Optional[Dict[str, Any]]:
        """获取指定task_id的状态信息"""
        with self._lock:
            status = self._status_instances.get(task_id)
            return status.get_status() if status else None

    def remove_status(self, task_id: str) -> bool:
        """移除已完成的任务状态实例"""
        with self._lock:
            if task_id in self._status_instances:
                del self._status_instances[task_id]
                return True
            return False

    def get_all_statuses(self) -> List[Dict[str, Any]]:
        """获取所有任务的状态信息"""
        with self._lock:
            return [
                status.get_status() 
                for status in self._status_instances.values()
            ]

    def get_active_statuses(self) -> List[Dict[str, Any]]:
        """获取所有活跃任务的状态信息"""
        with self._lock:
            return [
                status.get_status() 
                for status in self._status_instances.values()
                if status.is_active()
            ]

    def get_finished_statuses(self) -> List[Dict[str, Any]]:
        """获取所有已完成任务的状态信息"""
        with self._lock:
            return [
                status.get_status() 
                for status in self._status_instances.values()
                if status.is_finished()
            ]

    def cleanup_finished_tasks(self, older_than_seconds: int = 3600) -> int:
        """
        清理已完成的任务
        
        Args:
            older_than_seconds: 清理多少秒前完成的任务
            
        Returns:
            int: 清理的任务数量
        """
        with self._lock:
            to_remove = []
            current_time = time.time()
            
            for task_id, status in self._status_instances.items():
                if status.is_finished() and status._end_time:
                    # 计算任务结束时间
                    if current_time - status._end_time > older_than_seconds:
                        to_remove.append(task_id)
            
            # 删除过期的任务
            for task_id in to_remove:
                del self._status_instances[task_id]
            
            return len(to_remove)

    def _cleanup_old_tasks(self):
        """清理最老的任务，保持不超过最大历史数量"""
        if len(self._status_instances) > self._max_history:
            # 移除最老的已完成任务
            to_remove = []
            for task_id, status in self._status_instances.items():
                if status.is_finished():
                    to_remove.append(task_id)
                    if len(self._status_instances) - len(to_remove) <= self._max_history:
                        break
            
            # 如果已完成任务不够多，则清理最早的任务
            if len(self._status_instances) - len(to_remove) > self._max_history:
                # 按添加顺序清理最早的任务
                while len(self._status_instances) > self._max_history:
                    task_id, _ = self._status_instances.popitem(last=False)
                    to_remove.append(task_id)
            
            # 实际删除
            for task_id in to_remove:
                if task_id in self._status_instances:
                    del self._status_instances[task_id]

    def get_task_count(self) -> Tuple[int, int, int]:
        """
        获取任务统计信息
        
        Returns:
            Tuple[int, int, int]: (总任务数, 活跃任务数, 已完成任务数)
        """
        with self._lock:
            total = len(self._status_instances)
            active = sum(1 for status in self._status_instances.values() if status.is_active())
            finished = total - active
            return total, active, finished


# 全局状态管理器（单例）
global_status_manager = StatusManager(max_history=200)



# ==================== 辅助函数接口 ====================

def create_task(task_name: str = "") -> str:
    """
    创建新任务并返回任务ID
    
    Args:
        task_name: 可选的任务名称，用于描述性显示
        
    Returns:
        str: 任务ID，用于后续操作和查询
    """
    task_id, _ = global_status_manager.create_status(task_name)
    return task_id


def create_task_with_status(task_name: str = "") -> Tuple[str, ProcessingStatus]:
    """
    创建新任务并返回任务ID和状态对象
    
    Args:
        task_name: 可选的任务名称
        
    Returns:
        Tuple[str, ProcessingStatus]: (task_id, status_object)
    """
    return global_status_manager.create_status(task_name)


def get_task_status(task_id: str) -> Optional[Dict[str, Any]]:
    """
    获取任务状态信息
    
    Args:
        task_id: 任务ID
        
    Returns:
        Optional[Dict]: 任务状态字典，如果任务不存在则返回None
    """
    return global_status_manager.get_status_info(task_id)


def get_task_status_object(task_id: str) -> Optional[ProcessingStatus]:
    """
    获取任务状态对象（用于更新进度等操作）
    
    Args:
        task_id: 任务ID
        
    Returns:
        Optional[ProcessingStatus]: 任务状态对象
    """
    return global_status_manager.get_status(task_id)


def update_task_progress(task_id: str, progress: float, message: str = "") -> bool:
    """
    更新任务进度
    
    Args:
        task_id: 任务ID
        progress: 进度值（0-100）
        message: 消息
        
    Returns:
        bool: 是否成功更新
    """
    status = global_status_manager.get_status(task_id)
    if status:
        status.update_progress(progress, message)
        return True
    return False


def finish_task(task_id: str, success: bool = True, message: str = "", 
                error_details: str = "") -> bool:
    """
    标记任务完成
    
    Args:
        task_id: 任务ID
        success: 是否成功
        message: 完成消息
        error_details: 错误详情（如果失败）
        
    Returns:
        bool: 是否成功标记
    """
    status = global_status_manager.get_status(task_id)
    if status:
        status.finish(success, message, error_details)
        return True
    return False


def cancel_task(task_id: str, message: str = "任务已取消") -> bool:
    """
    取消任务
    
    Args:
        task_id: 任务ID
        message: 取消消息
        
    Returns:
        bool: 是否成功取消
    """
    status = global_status_manager.get_status(task_id)
    if status:
        status.cancel(message)
        return True
    return False


def pause_task(task_id: str) -> bool:
    """
    暂停任务
    
    Args:
        task_id: 任务ID
        
    Returns:
        bool: 是否成功暂停
    """
    status = global_status_manager.get_status(task_id)
    if status:
        status.pause()
        return True
    return False


def resume_task(task_id: str) -> bool:
    """
    恢复任务
    
    Args:
        task_id: 任务ID
        
    Returns:
        bool: 是否成功恢复
    """
    status = global_status_manager.get_status(task_id)
    if status:
        status.resume()
        return True
    return False


def remove_task(task_id: str) -> bool:
    """
    移除任务记录
    
    Args:
        task_id: 任务ID
        
    Returns:
        bool: 是否成功移除
    """
    return global_status_manager.remove_status(task_id)


def get_all_tasks() -> List[Dict[str, Any]]:
    """
    获取所有任务的状态信息
    
    Returns:
        List[Dict]: 所有任务状态列表
    """
    return global_status_manager.get_all_statuses()


def get_active_tasks() -> List[Dict[str, Any]]:
    """
    获取所有活跃任务的状态信息
    
    Returns:
        List[Dict]: 活跃任务状态列表
    """
    return global_status_manager.get_active_statuses()


def get_finished_tasks() -> List[Dict[str, Any]]:
    """
    获取所有已完成任务的状态信息
    
    Returns:
        List[Dict]: 已完成任务状态列表
    """
    return global_status_manager.get_finished_statuses()


def get_task_count() -> Tuple[int, int, int]:
    """
    获取任务统计信息
    
    Returns:
        Tuple[int, int, int]: (总任务数, 活跃任务数, 已完成任务数)
    """
    return global_status_manager.get_task_count()


def cleanup_old_tasks(older_than_seconds: int = 3600) -> int:
    """
    清理旧任务
    
    Args:
        older_than_seconds: 清理多少秒前完成的任务
        
    Returns:
        int: 清理的任务数量
    """
    return global_status_manager.cleanup_finished_tasks(older_than_seconds)


def clear_all_tasks():
    """清除所有任务"""
    global_status_manager.clear_all()