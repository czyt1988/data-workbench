#ifndef DADATAPYTHONBINDING_H
#define DADATAPYTHONBINDING_H
/**
@file DADataPythonBinding
@brief Python 绑定模块 da_data 的完整 API 清单与调用示例
@author ChenZongyan
@date   2025-11-30

@section py_overview  一、模块功能
da_data 模块把 C++ 的核心数据层暴露给 Python，使脚本能够：
- 直接读取/构造 pandas.DataFrame 或 pandas.Series 并封装为 DAData；
- 将 DAData 加入 DADataManager（支持撤销/重做）；
- 手动触发数据变更信号，驱动 Qt 界面刷新；
- 查询、删除、设置脏标志等常规管理操作。

@section py_class_list  二、导出类型一览

| Python 类 / 枚举        | 对应 C++ 类型                  | 说明 |
|-------------------------|--------------------------------|------|
| DAData                  | DA::DAData                     | 轻量包装，内部持有 shared_ptr<DAAbstractData> |
| DADataManager           | DA::DADataManager              | 数据管理中心，信号与槽、撤销栈 |
| DataChangeType          | DA::DADataManager::ChangeType  | 变更原因枚举 |

@section py_ctor  三、构造与转换
@subsection py_ctor_data  1. DAData 构造
- DAData()
创建空对象。
- DAData(pandas.DataFrame df)
将 pandas.DataFrame 封装为 DAData（内部类型标记为 TypePythonDataFrame）。
- DAData(pandas.Series ser)
将 pandas.Series 封装为 DAData（内部类型标记为 TypePythonSeries）。

@subsection py_convert  2. DAData → pandas 反向转换
@code
df = data.toPyObject()   # 返回原 pandas.DataFrame/Series 或 None
@endcode

@section py_member_data  四、DAData 成员
- getName() -> str
- setName(str) -> None
- getDescribe() -> str
- setDescribe(str) -> None

@section py_member_mgr  五、DADataManager 成员
@subsection py_add  1. 添加
- addDataFrame(df:pandas.DataFrame, name:str) -> None
快捷封装并 addData。
- addData(data:DAData) -> None
立即插入，无撤销。
- addData_(data:DAData) -> None
压入撤销栈，可 Ctrl+Z 回退。

@subsection py_query  2. 查询
- getDataCount() -> int
- getDataName(index:int) -> str
- getData(index:int) -> DAData
- getDataIndex(data:DAData) -> int   # 不存在返回 -1
- isDirty() -> bool
- setDirtyFlag(on:bool) -> None

@subsection py_remove  3. 删除
- removeData(data:DAData) -> None    # 无撤销
- removeData_(data:DAData) -> None   # 可撤销

@subsection py_signal  4. 手动通知
- notifyDataChangedSignal(data:DAData, changeType:DataChangeType) -> None
当 Python 脚本直接修改了底层 pandas 数据后，调用此函数驱动界面刷新。

@section py_enum  六、DataChangeType 枚举
| 成员名 | 含义 |
|--------|------|
| Name        | 数据名称改变 |
| Describe    | 数据描述改变 |
| Value       | 数据值（DataFrame/Series 内容）改变 |
| ColumnName  | 仅 DataFrame 列名改变 |

@section py_example  七、完整示例

@code{.py}
import pandas as pd
import da_data

# 1. 创建/读取 DataFrame
df = pd.read_csv("example.csv")

# 2. 构造 DAData 并命名
data = da_data.DAData(df)
data.setName("csv_example")
data.setDescribe("Imported from example.csv")

# 3. 加入管理器（支持撤销）
mgr = da_data.DADataManager()      # 实际应从 C++ 主程序注入
mgr.addData_(data)

# 4. Python 侧直接修改 DataFrame
df.loc[0, 'value'] = 999           # 直接改底层 pandas

# 5. 通知界面刷新
mgr.notifyDataChangedSignal(data, da_data.DataChangeType.Value)

# 6. 查询与删除
print("count =", mgr.getDataCount())
idx = mgr.getDataIndex(data)
same_data = mgr.getData(idx)
mgr.removeData_(same_data)         # Ctrl+Z 可回退
@endcode

@section py_thread  八、线程与生命周期
- 所有函数默认持有 GIL，Qt GUI 线程直接调用安全。
- 若脚本运行于后台线程，请使用 py::gil_scoped_release 或在 Qt 侧排队槽函数。
- DAData 返回的临时对象被 Python 持有期间，C++ 侧引用计数 +1，不会意外析构；但仍建议不要在删除数据后继续访问 Python 对象。

@section py_limit  九、当前限制
- removeData/removeData_ 要求目标 DAData 必须已归属本管理器，否则静默无操作。
- setDirtyFlag 仅修改内部标志，不会自动更新主窗口标题或保存按钮状态，需要主程序额外连接信号。
*/

// pybind11


// DA
#include "DADataManager.h"
#include "DAData.h"
#include "pandas/DAPyDataFrame.h"


/**
 *@brief 把 pandas.DataFrame 转成 DAPyDataFrame
 *
 *此函数主要是给python里调用
 */
DA::DAPyDataFrame pyDataFrameToDAPyDataFrame(pybind11::object df);

// 包装 addDataFrame
/**
 * @brief 把dataframe添加到datamanager
 * @param mgr
 * @param df
 * @param name
 */
void addDataFrameFromPy(DA::DADataManager& mgr, pybind11::object df, const std::string& name);

#endif  // DADATAPYTHONBINDING_H
