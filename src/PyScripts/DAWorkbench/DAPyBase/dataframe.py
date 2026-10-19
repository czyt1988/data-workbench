# -*- coding: utf-8 -*-
import os
import sys
from typing import List, Dict, Tuple, Optional, Union
# 根据Python版本动态导入Literal
if sys.version_info >= (3, 8):
    from typing import Literal
else:
    from typing_extensions import Literal
# 在Python 3.10及之后的版本中引入了|操作符用于类型联合，而在此之前的版本并不支持这种语法,为了支持win7（python3.7）,
# 这里不应使用|操作符可以使用Union替换|，Literal是3.8之后才支持，这里也不应该引入
# 如果确实是在Python 3.7环境下并且需要使用Literal，可以引入typing_extensions包，from typing_extensions import Literal

from pandas._typing import Axis, Scalar
import pandas as pd
import numpy as np
from DAWorkbench.DAPyBase.da_logger import log_function_call  # type: ignore # 引入装饰器
import copy

'''
本文件da_打头的变量和函数属于da系统的默认函数，如果改动会导致da系统异常

此文件封装dataframe的操作
'''


@log_function_call(slow=True)
def da_drop_irow(df: pd.DataFrame, index: List[int]):
    '''
    传入行索引，并把对应的行删除
    :param df: pd.DataFrame
    :param index: 删除的行索引
    :return: 此函数不返回值，直接改变df
    '''
    i = df.index[index]
    df.drop(index=i, axis=0, inplace=True)


@log_function_call(slow=True)
def da_drop_icolumn(df: pd.DataFrame, index: List[int]):
    '''
    传入列索引，并把对应的列删除
    :param df:  pd.DataFrame
    :param index: 删除的列索引
    :return: 此函数不返回值，直接改变df
    '''
    cols = df.columns[index]
    df.drop(cols, axis=1, inplace=True)


@log_function_call(slow=True)
def da_drop_na(df: pd.DataFrame, axis: int = 0, how: Literal['any', 'all'] = 'any', index: Optional[List[int]] = None, thresh: Optional[int] = None):
    '''
    删除dataframe中的nan值
    :param df: pd.DataFrame
    :param axis: 0为行，1为列
    :param how: any为只要有nan就删除，all为全为nan才删除
    :param index: 列索引
    :param thresh: 可选参数，表示在删除之前需要满足的非缺失值的最小数量。如果行或列中的非缺失值数量小于等于thresh，则会被删除
    :return: 此函数不返回值，直接改变df
    '''
    subset = None
    if index is not None:
        if axis == 0:
            subset = df.columns[index]
        else:
            subset = df.index[index]
    if thresh is not None:
        df.dropna(axis=axis, subset=subset, thresh=thresh, inplace=True)
    else:
        df.dropna(axis=axis, how=how, subset=subset, inplace=True)


@log_function_call(slow=True)
def da_drop_duplicates(df: pd.DataFrame, keep: Literal['first', 'last'] = 'first', index: Optional[List[int]] = None, ignore_index=False):
    '''
    删除dataframe中的重复值
    :param df: pd.DataFrame
    :param axis: 0为行，1为列
    :param keep: first表示保留第一个重复值，last表示保留最后一个重复值，默认为first
    :param index: 列索引
    :param ignore_index: 是否忽略索引，默认为False
    :return: 此函数不返回值，直接改变df
    '''
    subset = None
    if index is not None:
        subset = df.columns[index].tolist()
    df.drop_duplicates(subset=subset, keep=keep,
                       ignore_index=ignore_index, inplace=True)


@log_function_call(slow=True)
def da_fill_na(df: pd.DataFrame, value: Union[Scalar, dict, None] = None, axis: Optional[Axis] = None, limit: Optional[int] = None, downcast: Optional[dict] = None):
    '''
    填充dataframe中的nan值
    :param df: pd.DataFrame
    :param value: 用于填充缺失值的值或字典。可以是一个标量值、字典、Series 或 DataFrame
    :param axis: 填充的轴方向，0 或 'index' 表示按行填充，1 或 'columns' 表示按列填充。
    :param limit: 限制向前或向后填充的最大数量
    :param downcast:可选的字典，指定向下转型操作（例如将浮点数转换为整数等）。
    :return: 此函数不返回值，直接改变df
    :example:
    df = pd.DataFrame([[np.nan, 2, np.nan, 0],
                    [3, 4, np.nan, 1],
                    [np.nan, np.nan, np.nan, np.nan],
                    [np.nan, 3, np.nan, 4]],
                    columns=list("ABCD"))
    values = {"A": 0, "B": 1, "C": 2, "D": 3}
    df.fillna(value=values)
    '''
    df.fillna(axis=axis, value=value, downcast=downcast, inplace=True)


@log_function_call(slow=True)
def da_ffill_na(df: pd.DataFrame, axis: Optional[Axis] = None, limit: Optional[int] = None, downcast: Optional[dict] = None):
    '''
    填充dataframe中的nan值
    :param df: pd.DataFrame
    :param axis: 填充的轴方向，0 或 'index' 表示按行向下填充，1 或 'columns' 表示按列向右填充。
    :param limit: 限制向后填充的最大数量
    :param downcast:可选的字典，指定向下转型操作（例如将浮点数转换为整数等）。
    :return: 此函数不返回值，直接改变df
    '''
    df.ffill(axis=axis, inplace=True, limit=limit, downcast=downcast)


@log_function_call(slow=True)
def da_bfill_na(df: pd.DataFrame, axis: Optional[Axis] = None, limit: Optional[int] = None, limit_area: Optional[Literal['inside', 'outside']] = None):
    '''
    填充dataframe中的nan值
    :param df: pd.DataFrame
    :param axis: 填充的轴方向,0表示按行向上填充,1表示按列向左填充。
    :param limit: 限制向前填充的最大数量
    :param limit_area: 字符串，默认为 None。可选值为 None、inside 或 outside。inside 表示只填充被有效值包围的缺失值，outside 表示只填充在有效值之前或之后的缺失值。
    :return: 此函数不返回值，直接改变df
    '''
    df.bfill(axis=axis, inplace=True, limit=limit, limit_area=limit_area)


@log_function_call(slow=True)
def da_nstd_filter_outlier(df: pd.DataFrame, n=3, axis: Optional[int] = None, index: Optional[List[int]] = None):
    """
    使用n倍标准差法过滤DataFrame的行或列（直接在原数据上修改）
    :param df: 输入的pd.DataFrame
    :param n: 标准差的倍数，范围是0.1~10，默认为3
    :param axis: 过滤方向，1表示基于选中列过滤行(默认)，0表示基于选中行过滤列
    :param index: 需要过滤的行或列的索引列表，如果为None，则不进行过滤
    :return: 过滤后的DataFrame
    """
    # 如果没有指定要过滤的列/行，直接返回原DataFrame
    if index is None or len(index) == 0:
        return df

    # 检查所选列是否全是字符串类型，如果是则跳过计算
    selected_columns = df.iloc[:, index]
    if all(selected_columns.dtypes == "object"):  # Pandas 中字符串列的类型通常是 'object'
        return df

    # 计算均值和标准差
    if axis == 1:  # 计算选中列的值，过滤行
        data = df.iloc[:, index]
        mean = data.mean(axis=0)  # 沿着行的方向，计算每一列的平均值
        std = data.std(axis=0)    # 沿着行的方向，计算每一列的标准差

        # 利用广播一次性计算所有列的掩码
        lower = mean - n * std     # shape: (n_cols,)
        upper = mean + n * std     # shape: (n_cols,)

        # 广播比较：data shape (n_rows, n_cols) vs lower/upper shape (n_cols,)
        # 每行所有列都在 [lower, upper] 范围内才保留
        mask = (data >= lower) & (data <= upper)  # shape: (n_rows, n_cols)
        keep_rows = mask.all(axis=1)  # 所有选中列都满足才保留

        # 直接删除不符合条件的行
        df.drop(df.index[~keep_rows], inplace=True)


@log_function_call(slow=True)
def da_fill_interpolate(df: pd.DataFrame, method: Literal['spline', 'polynomial'] = 'spline', order: int = 1, axis: int = 0, limit: Optional[int] = None,
                        limit_direction: Literal["forward", "backward", "both"] = "forward", limit_area: Literal["inside", "outside", None] = None, downcast: Optional[dict] = None):
    '''
    插值法填充dataframe中的nan值
    :param df: pd.DataFrame。
    :param method: 用于插值法填充缺失值的方法。spline代表线程插值，polynomial代表多项式插值。
    :param order: 插值多项式的次数。
    :param axis: 填充的轴方向，0 或 'index' 表示按行填充，1 或 'columns' 表示按列填充。
    :param limit: 限制向前或向后填充的最大数量
    :param limit_direction: 限制插值的方向，默认为'forward'。
    :param limit_area: 限制插值的区域，在有效值的内部或外部，默认为None,。
    :param downcast:可选的字典，指定向下转型操作（例如将浮点数转换为整数等）。
    :return: 此函数不返回值，直接改变df
    '''
    df.interpolate(method=method, order=order, axis=axis,
                   downcast=downcast, inplace=True)


@log_function_call(slow=True)
def da_clip_outlier(df: pd.DataFrame, lower: Optional[float] = None, upper: Optional[float] = None, axis: int = 0):
    '''
    替换dataframe的异常值
    :param df: pd.DataFrame。
    :param lower: 所有小于 lower 的值会被替换为 lower。如果为 None，则不应用下界。
    :param upper: 所有大于 upper 的值会被替换为 upper。如果为 None，则不应用上界。
    :param axis: 填充的轴方向，0 或 'index' 表示按行填充，1 或 'columns' 表示按列填充。
    :return: 此函数不返回值，直接改变df
    '''
    df.clip(lower=lower, upper=upper, axis=axis, inplace=True)


@log_function_call(slow=True)
def da_create_pivot_table(df: pd.DataFrame, values=None, index=None, columns=None, aggfunc: Literal['mean', 'sum', 'size'] = 'mean',
                          margins: bool = False, margins_name: str = "All", sort: bool = False):
    '''
    创建数据透视表
    :param values: 要进行汇总的数据值
    :param index: 确定行参数
    :param columns: 确定列参数
    :param aggfunc: 要计算的函数，mean求均值、sum求和、size计算个数
    :param margins: 行列数据的统计
    :param margins_name: 行列数据的统计表头名
    :param sort: 聚合后的结果排序
    :return: 数据聚合后的DataFrame
    '''
    return df.pivot_table(values=values, index=index, columns=columns, aggfunc=aggfunc, margins=margins, margins_name=margins_name, sort=sort)


@log_function_call(slow=True)
def da_eval_datas(df: pd.DataFrame, expr: str, parser: str = "pandas", engine: Optional[str] = None, local_dict=None, global_dict=None, resolvers=(), level: int = 0, target=None):
    '''
    pandas.DataFrame.eval的wrapper（expr 为完整赋值表达式，如 "C = A + B"）

    注意：target=None 时不能显式传给 df.eval——DataFrame.eval 内部
    kwargs["target"] = kwargs.pop("target", self)，显式 None 会顶掉默认 self，
    赋值表达式将抛 "cannot assign without a target object"。计算列求值请用
    @ref da_eval_column（RHS 求值 + 按列名赋值，健壮支持含空格目标列名）。
    '''
    if target is None:
        df.eval(expr, inplace=True, parser=parser, engine=engine,
                local_dict=local_dict, global_dict=global_dict,
                resolvers=resolvers, level=level)
    else:
        df.eval(expr, inplace=True, parser=parser, engine=engine,
                local_dict=local_dict, global_dict=global_dict,
                resolvers=resolvers, level=level, target=target)


@log_function_call(slow=True)
def da_eval_column(df: pd.DataFrame, target_column: str, body: str, parser: str = "pandas", engine: Optional[str] = None):
    '''
    计算列求值（004 T1.23）：把 RHS 表达式 body 求值后写入 target_column 列

    与 da_eval_datas 的赋值语法不同——本函数用 df.eval(body) 只求值 RHS（不做
    赋值），再按列名 df[target_column] = 结果 赋值。这样目标列名可含空格/特殊字符
    （pandas 3.x 下 df.eval 的反引号赋值 LHS 会产出 BACKTICK_QUOTED_STRING_* 损坏列名，
    故不走赋值语法）。body 里对含空格列的引用仍用反引号（读取路径正常）。

    求值前对目标列做旧值快照，返回 (had_column, old_series)，供 C++ 侧构建列级撤销
    记录（D4 值记录）：
    - had_column=True: old_series 为求值前的列数据副本（重算场景，undo 还原旧列）
    - had_column=False: old_series 为 None（新建列场景，undo 删除该列）
    求值失败（语法错误/引用不存在的列）抛异常，快照不返回，数据不变（不静默清空）。

    :param target_column: 目标列名（重名时快照/赋值第一个同名列位置）
    :param body: RHS 表达式体（不含 "目标列 ="）
    :return: (had_column, old_series)
    '''
    locs = [i for i, c in enumerate(df.columns) if str(c) == str(target_column)]
    if locs:
        snapshot = (True, df.iloc[:, locs[0]].copy())
    else:
        snapshot = (False, None)
    # 只求值 RHS（inplace=False 返回 Series/标量），再按列名赋值
    result = df.eval(body, inplace=False, parser=parser, engine=engine)
    if locs:
        df.iloc[:, locs[0]] = result
    else:
        df[target_column] = result
    return snapshot


def da_needs_backtick(name: str) -> bool:
    '''
    列名在 pandas eval 表达式里是否需要反引号包裹（004 T1.23 R20）

    合法 Python 标识符且非关键字可直接裸写；含空格/特殊字符/以数字开头/关键字
    （如 "my col"、"a-b"、"2024"、"in"）必须用反引号，否则 eval 解析失败或语义错误
    '''
    import keyword
    s = str(name)
    return not (s.isidentifier() and not keyword.iskeyword(s))


@log_function_call(slow=True)
def da_make_eval_expr(target: str, body: str) -> str:
    '''
    构建 pandas eval 完整赋值表达式："目标列 = 表达式体"（004 T1.23）

    目标列名含空格/特殊字符时自动加反引号（R20）；表达式体里的列名引用由
    调用方（对话框列名点选插入）负责反引号包裹。预览与执行共用本函数（同一算子）。
    '''
    t = '`%s`' % target if da_needs_backtick(target) else str(target)
    return '%s = %s' % (t, body)


@log_function_call(slow=True)
def da_expr_depends(expr: str, columns):
    '''
    提取表达式引用的列名（与实际列的交集，按 columns 顺序返回，004 T1.23 依赖追踪）

    实现：反引号段先替换为合成标识符，再用 ast 提取 Name 节点（字符串字面量、
    数字常量不会误报），合成标识符映射回原列名后与 columns 求交集。
    函数名（如 abs、sin）不在 columns 里，天然被交集过滤。
    表达式无法解析（ast SyntaxError）时抛 SyntaxError，C++ 侧转校验失败提示。

    :param expr: 表达式体（赋值右侧，不含 "目标列 ="）
    :param columns: 实际列名序列
    :return: 被引用的列名列表（按 columns 顺序，去重）
    '''
    import re
    import ast
    mapping = {}

    def _repl(m):
        key = '_dabt%d_' % len(mapping)
        mapping[key] = m.group(1)
        return key

    sanitized = re.sub(r'`([^`]*)`', _repl, str(expr))
    tree = ast.parse(sanitized, mode='eval')  # SyntaxError 由调用方捕获
    names = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Name):
            names.add(mapping.get(node.id, node.id))
    return [str(c) for c in columns if str(c) in names]


@log_function_call(slow=True)
def da_head_copy(df: pd.DataFrame, n: int) -> pd.DataFrame:
    '''
    取头部前 n 行的独立副本（对话框实时预览用，004：预览与执行同一算子，
    在副本上调用同一算子函数，不触碰原表）
    '''
    return df.head(max(0, int(n))).copy()


@log_function_call(slow=True)
def da_get_column_copy(df: pd.DataFrame, col: int) -> pd.Series:
    '''
    按列位置取一列的独立副本（分列/合列命令的列级撤销快照，004 T1.9 D4）

    :raises IndexError: col 越界
    '''
    if col < 0 or col >= df.shape[1]:
        raise IndexError("column index %d out of range [0, %d)" % (col, df.shape[1]))
    return df.iloc[:, col].copy()


@log_function_call(slow=True)
def da_set_column(df: pd.DataFrame, name: str, series: pd.Series):
    '''
    按列名把 series 写回列（计算列重算命令的 undo 还原旧列，004 T1.23）

    列必须已存在（重算场景目标列恒存在）；按 index 对齐写回。
    重名列时 pandas 会写全部同名列——计算列禁止重名目标（对话框校验），不触发此分支。
    '''
    df[name] = series


def _da_try_numeric(values: pd.Series) -> pd.Series:
    '''
    尝试把 object 列解析为数值列（004 T1.9 类型推断）：
    空串先按缺失处理；全部非空值都能解析才转数值，否则原样返回字符串列。
    '''
    work = values.replace('', np.nan)
    converted = pd.to_numeric(work, errors='coerce')
    # 非空值全部解析成功（NaN 位置一致）才采用数值列
    if converted.isna().equals(work.isna()):
        return converted
    return values


@log_function_call(slow=True)
def da_split_column(df: pd.DataFrame, col: int, mode: str = 'char', delimiter: str = ',',
                    pattern: Optional[str] = None, widths=None,
                    max_columns: Optional[int] = None, overflow: str = 'discard',
                    infer_numeric: bool = True, new_names=None) -> List[str]:
    '''
    就地分列（004 T1.9）：把第 col 列拆分，结果列插入原列右侧；原列保留。
    预览与执行共用本函数（对话框对 da_head_copy 副本调用，所见即所得）。

    落位规则：结果列依次插在 col+1 .. col+n（原列右侧，原列不动）。

    :param col: 源列位置
    :param mode: 'char'=分隔符（单字符/字符串）| 'regex'=正则捕获组 | 'width'=固定宽度
    :param delimiter: char 模式分隔符（非空）
    :param pattern: regex 模式正则（必须含捕获组，列数=组数）
    :param widths: width 模式宽度列表（正整数，如 [3,5,8]，自串首按累计宽度切片）
    :param max_columns: 结果列数上限（None=不限：char 取最多拆分行、regex 取组数、width 取 len(widths)）
    :param overflow: 超出 max_columns（或 width 模式串长超总宽）时的策略：
        'discard'=丢弃超出部分；
        'keep'=保留原文——char/width 把该行剩余未拆文本并入最后一个结果列，regex 把整格原文放入最后一列；
        'error'=抛 ValueError（数据不变）
    :param infer_numeric: 结果列尝试数值解析（整列非空值全可解析才转数值；空串按缺失）
    :param new_names: 结果列名（长度必须与实际结果列数一致；None 自动 "{源列名}_{i}"，i 从 1）
    :return: 实际插入的结果列名列表（按插入顺序）
    :raises ValueError: 参数/溢出策略错误；IndexError: col 越界；re.error: 正则无效
    '''
    import re as _re
    if col < 0 or col >= df.shape[1]:
        raise IndexError("column index %d out of range [0, %d)" % (col, df.shape[1]))
    src = df.iloc[:, col]
    src_name = str(df.columns[col])
    # 统一转字符串视图（NaN 行保持 None → 结果各列为 NaN）
    s = src.astype('object').where(src.notna(), None)
    s = s.map(lambda v: None if v is None else str(v))
    n_max = None if (max_columns is None or int(max_columns) <= 0) else int(max_columns)

    if mode == 'char':
        if not delimiter:
            raise ValueError("delimiter must not be empty in char mode")
        split_df = s.str.split(delimiter, expand=True)
        natural = split_df.shape[1]
        n = natural if n_max is None else min(natural, n_max)
        if n <= 0:
            raise ValueError("nothing to split")
        if n_max is not None and natural > n_max:
            if overflow == 'error':
                raise ValueError("split produces %d columns, exceeding max_columns=%d" % (natural, n_max))
            if overflow == 'keep':
                # 限制拆分次数：剩余部分并入最后一列
                split_df = s.str.split(delimiter, n=n - 1, expand=True)
        split_df = split_df.reindex(columns=range(n))
        parts = split_df
    elif mode == 'regex':
        if not pattern:
            raise ValueError("pattern must not be empty in regex mode")
        _re.compile(pattern)  # 无效正则尽早抛 re.error
        extracted = s.str.extract(pattern)
        groups = extracted.shape[1]
        if groups == 0:
            raise ValueError("regex pattern has no capture groups")
        n = groups if n_max is None else min(groups, n_max)
        if n_max is not None and groups > n_max:
            if overflow == 'error':
                raise ValueError("regex has %d capture groups, exceeding max_columns=%d" % (groups, n_max))
            if overflow == 'keep':
                extracted = extracted.iloc[:, :n].astype(object)
                extracted.iloc[:, n - 1] = s  # 整格原文放入最后一列
            else:
                extracted = extracted.iloc[:, :n]
        parts = extracted.reindex(columns=range(n))
    elif mode == 'width':
        ws = [int(w) for w in (widths if widths is not None else [])]
        if not ws or any(w <= 0 for w in ws):
            raise ValueError("widths must be a non-empty list of positive ints")
        if n_max is not None and len(ws) > n_max:
            if overflow == 'error':
                raise ValueError("widths define %d columns, exceeding max_columns=%d" % (len(ws), n_max))
            ws = ws[:n_max]
        n = len(ws)
        total = sum(ws)
        slen = s.str.len()
        if overflow == 'error' and bool((slen > total).any()):
            raise ValueError("some values are longer than total width %d" % total)
        parts = {}
        start = 0
        for i, w in enumerate(ws):
            if overflow == 'keep' and i == n - 1:
                parts[i] = s.str.slice(start)  # 最后一列吃到串尾（保留剩余原文）
            else:
                parts[i] = s.str.slice(start, start + w)
            start += w
        parts = pd.DataFrame(parts, index=df.index).reindex(columns=range(n))
        # 空串（越界切片）按缺失处理
        parts = parts.replace('', np.nan)
    else:
        raise ValueError("unknown split mode: %s" % mode)

    # 结果列名
    if new_names is None:
        names = ['%s_%d' % (src_name, i + 1) for i in range(n)]
    else:
        names = [str(x) for x in new_names]
        if len(names) != n:
            raise ValueError("new_names length %d != result column count %d" % (len(names), n))
    # 类型推断（可关）：逐列尝试数值解析后重建结果帧（保持 int64/float64 真实 dtype；
    # pandas 3.x 的 str dtype 列不接受数值回填，不能 iloc 原地赋值）
    if infer_numeric:
        parts = pd.DataFrame({i: _da_try_numeric(parts.iloc[:, i]) for i in range(n)}, index=df.index)
    # 就地插入原列右侧（col+1 .. col+n）
    for i in range(n):
        df.insert(col + 1 + i, names[i], parts.iloc[:, i])
    return names


@log_function_call(slow=True)
def da_groupby_aggregate(df: pd.DataFrame, keys, values, funcs) -> pd.DataFrame:
    '''
    分组聚合（004 T1.22）：产出长格式聚合结果表（新 DataFrame，不改原表）

    聚合函数集合与透视表（da_create_pivot_table）对齐：'mean' / 'sum' / 'size'
    （要扩展须两处同扩）。多键多列：keys 为分组键列名列表，values/funcs 一一对应
    （同一列可用不同函数出现多次）。结果 reset_index 成普通列（长格式），聚合列
    命名 "{值列}_{函数}"；size 只计组内行数（对应 values 项仅作命名占位）。
    分组键含 NaN 时保留 NaN 组（dropna=False，SQL GROUP BY 语义）。

    :param keys: 分组键列名列表（非空）
    :param values: 聚合列名列表（与 funcs 等长）
    :param funcs: 聚合函数列表（'mean'/'sum'/'size'）
    :return: 聚合结果 DataFrame（行数 = 分组数）
    :raises ValueError: 参数为空/长度不一致/函数不支持/列名不存在
    '''
    keys = [str(k) for k in keys]
    values = [str(v) for v in values]
    funcs = [str(f) for f in funcs]
    if not keys:
        raise ValueError("keys must not be empty")
    if not values or len(values) != len(funcs):
        raise ValueError("values/funcs must be non-empty and of the same length")
    allowed = ('mean', 'sum', 'size')
    columns = [str(c) for c in df.columns]
    for k in keys:
        if k not in columns:
            raise ValueError("key column not found: %s" % k)
    for f in funcs:
        if f not in allowed:
            raise ValueError("unsupported agg func: %s (allowed: %s)" % (f, '/'.join(allowed)))
    for v in values:
        if v not in columns:
            raise ValueError("value column not found: %s" % v)
    g = df.groupby(keys, dropna=False, sort=True)
    parts = []
    for v, f in zip(values, funcs):
        if f == 'size':
            s = g.size().rename('%s_size' % v)
        else:
            s = g[v].agg(f).rename('%s_%s' % (v, f))
        parts.append(s)
    return pd.concat(parts, axis=1).reset_index()


@log_function_call(slow=True)
def da_group_count(df: pd.DataFrame, keys) -> int:
    '''
    分组数（004 T1.22 对话框「预估结果行数」）：与 da_groupby_aggregate 同口径
    （dropna=False 保留 NaN 组）。
    '''
    return int(df.groupby([str(k) for k in keys], dropna=False).ngroups)


@log_function_call(slow=True)
def da_transpose(df: pd.DataFrame) -> pd.DataFrame:
    '''
    转置（004 T1.27）：行列互换，产出**新** DataFrame（不改原表，R14 非破坏默认）

    结果第一列名为 "index"、内容是原列名（Excel 转置语义：表头变第一列）；
    其余列名 = 原行索引值（统一转字符串，避免非字符串列名进入表格模型）。
    混合 dtype 转置后为 object 列（pandas 语义），进图表/工作流前可再转类型。
    '''
    out = df.T.reset_index(drop=False)
    out.columns = [str(c) for c in out.columns]
    return out


@log_function_call(slow=True)
def da_melt(df: pd.DataFrame, id_vars, value_vars, var_name: str = 'variable', value_name: str = 'value') -> pd.DataFrame:
    '''
    宽转长 melt（004 T1.27）：产出**新** DataFrame（不改原表，R14 非破坏默认）

    :param id_vars: 保留列名列表（可空 = 无保留列）
    :param value_vars: 展开列名列表（空/None = 除保留列外全部列，pandas 语义）
    :param var_name: 变量名列名（默认 "variable"）
    :param value_name: 值列名（默认 "value"）
    :return: 长格式结果，行数 = 原行数 × 展开列数（规模守卫由调用侧预估）
    '''
    idl = [str(c) for c in id_vars] if id_vars is not None and len(list(id_vars)) > 0 else None
    vvl = [str(c) for c in value_vars] if value_vars is not None and len(list(value_vars)) > 0 else None
    out = pd.melt(df, id_vars=idl, value_vars=vvl,
                  var_name=str(var_name) if var_name else 'variable',
                  value_name=str(value_name) if value_name else 'value')
    out.columns = [str(c) for c in out.columns]
    return out


@log_function_call(slow=True)
def da_merge_columns(df: pd.DataFrame, cols, sep: str = '', skip_na: bool = True, name: str = 'merged'):
    '''
    就地合并列（004 T1.9）：按 cols 给定顺序把各列值拼接为新列，
    插入到最左侧源列的位置，并删除全部源列（落位规则：替换被合并列的位置）。
    预览与执行共用本函数。

    :param cols: 源列位置列表（顺序=拼接顺序，须 >= 2 列）
    :param sep: 分隔符（可空串）
    :param skip_na: True 跳过缺失值（NaN/NaT/None）且不留多余分隔符；False 缺失按空串参与拼接
    :param name: 结果列名
    :return: (插入位置 pos, 结果列名 name)
    :raises ValueError: cols 不足两列/重名结果列由调用方校验；IndexError: 位置越界
    '''
    cols = [int(c) for c in cols]
    if len(cols) < 2:
        raise ValueError("merge requires at least 2 columns")
    ncols = df.shape[1]
    if any(c < 0 or c >= ncols for c in cols):
        raise IndexError("merge column index out of range [0, %d)" % ncols)
    pos = min(cols)
    strs = []
    for c in cols:
        sv = df.iloc[:, c]
        strs.append(sv.astype(str).where(sv.notna(), None))  # 缺失 → None
    if skip_na:
        acc = strs[0].fillna('')
        has = strs[0].notna()
        for s in strs[1:]:
            notna = s.notna()
            both = has & notna
            acc = pd.Series(np.where(both, acc + sep + s.fillna(''),
                                     np.where(has, acc, s.fillna(''))),
                            index=df.index, dtype=object)
            has = has | notna
        merged = acc.where(has, np.nan)  # 全缺失行 → NaN
    else:
        acc = strs[0].fillna('')
        for s in strs[1:]:
            acc = acc + sep + s.fillna('')
        merged = acc
    # 删除源列（按位置降序，避免位移；与 da_drop_icolumn 同一路径）
    for c in sorted(cols, reverse=True):
        da_drop_icolumn(df, [c])
    df.insert(pos, name, merged)
    return (pos, name)


@log_function_call(slow=True)
def da_query_datas(df: pd.DataFrame, expr: Optional[str]):
    '''
    pandas.DataFrame.query的wrapper
    :param df: pd.DataFrame。
    :param expr:字符串形式的筛选条件。
    :return: 此函数不返回值，直接改变df
    '''
    df.query(expr, inplace=True)


def _da_resolve_bound(rng, n: int):
    '''
    解析范围闭区间：None → 全范围 [0, n-1]；(lo, hi) 端点可为 None（表示该端不限），越界截断
    :return: (lo, hi)；lo > hi 表示空范围
    '''
    if n <= 0:
        return 0, -1
    if rng is None:
        return 0, n - 1
    lo = 0 if rng[0] is None else max(0, int(rng[0]))
    hi = (n - 1) if rng[1] is None else min(n - 1, int(rng[1]))
    return lo, hi


def _da_is_text_column(values) -> bool:
    '''是否文本类列（object/str/string dtype），文本匹配与正则只作用于这类列（§4.2 v1）'''
    dt = values.dtype
    if dt == np.dtype('O'):
        return True
    try:
        return bool(pd.api.types.is_string_dtype(dt))
    except TypeError:
        return False


def _da_parse_numeric_keyword(find):
    '''关键字解析为数值；不可解析返回 None'''
    try:
        return float(find)
    except (TypeError, ValueError):
        return None


def _da_column_find_mask(values, find: str, case_sensitive: bool, use_regex: bool, whole_cell: bool):
    '''
    单列查找命中掩码（布尔 ndarray，与 values 等长）。

    匹配语义（§4.2 v1，在原始值上匹配）：
    - 文本列（object/str）：正则 → str.contains(regex=True)/str.fullmatch；
      非正则 → 子串包含（字面量）或整格相等（whole_cell）；大小写开关生效；NaN 不命中
    - 数值列（非正则时）：关键字可解析为数值则整格相等命中（与旧 da_search_data 的
      数值等值语义一致）；正则模式跳过数值列（正则只作用文本列，避免 dtype 破坏）
    - 日期/布尔等其它类型列：v1 不命中
    '''
    n = len(values)
    if use_regex:
        if not _da_is_text_column(values):
            return np.zeros(n, dtype=bool)
        if whole_cell:
            mask = values.str.fullmatch(find, case=case_sensitive, na=False)
        else:
            mask = values.str.contains(find, case=case_sensitive, regex=True, na=False)
        return np.asarray(mask, dtype=bool)
    if _da_is_text_column(values):
        if whole_cell:
            if case_sensitive:
                mask = values.eq(find)
            else:
                mask = values.str.lower().eq(str(find).lower())
        else:
            mask = values.str.contains(find, case=case_sensitive, regex=False, na=False)
        return np.asarray(mask, dtype=bool)
    dt = values.dtype
    if pd.api.types.is_numeric_dtype(dt) and dt.kind != 'b':
        num = _da_parse_numeric_keyword(find)
        if num is None:
            return np.zeros(n, dtype=bool)
        return np.asarray(values.to_numpy() == num, dtype=bool)
    return np.zeros(n, dtype=bool)


def _da_find_hits(df: pd.DataFrame, find: str, case_sensitive: bool, use_regex: bool, whole_cell: bool, row_range, col_range):
    '''
    范围内扫描命中（全表扫描必须走 Python：命中可能落在滑动窗外，C++ 侧拿不到，§4.2）
    :return: (rows, cols) int64 数组，行主序（先按行再按列，与 Excel 查找顺序一致）
    '''
    r_lo, r_hi = _da_resolve_bound(row_range, len(df))
    c_lo, c_hi = _da_resolve_bound(col_range, df.shape[1])
    rows_list = []
    cols_list = []
    for c in range(c_lo, c_hi + 1):
        values = df.iloc[r_lo:r_hi + 1, c]
        mask = _da_column_find_mask(values, find, case_sensitive, use_regex, whole_cell)
        idx = np.flatnonzero(mask)
        if idx.size:
            rows_list.append(idx + r_lo)
            cols_list.append(np.full(idx.size, c, dtype=np.int64))
    if not rows_list:
        return np.empty(0, dtype=np.int64), np.empty(0, dtype=np.int64)
    rows = np.concatenate(rows_list)
    cols = np.concatenate(cols_list)
    order = np.lexsort((cols, rows))  # 行主序
    return rows[order], cols[order]


def _da_find_hits_equality(df: pd.DataFrame, data, row_range, col_range):
    '''
    旧等值比较路径（data 为非字符串：数值/NaN 等），保持旧 da_search_data 语义：
    NaN 用 pd.isna 向量化、普通值用 ==
    '''
    r_lo, r_hi = _da_resolve_bound(row_range, len(df))
    c_lo, c_hi = _da_resolve_bound(col_range, df.shape[1])
    if r_hi < r_lo or c_hi < c_lo:
        return np.empty(0, dtype=np.int64), np.empty(0, dtype=np.int64)
    sub = df.iloc[r_lo:r_hi + 1, c_lo:c_hi + 1]
    if pd.isna(data):
        mask = sub.isna()
    else:
        try:
            mask = (sub == data)
        except Exception:
            # 类型不兼容时的降级处理（pandas>=2.1 用 map，旧版用 applymap）
            if hasattr(sub, 'map'):
                mask = sub.map(lambda x: x == data)
            else:
                mask = sub.applymap(lambda x: x == data)
    rr, cc = np.asarray(mask).nonzero()  # nonzero 即行主序
    return rr + r_lo, cc + c_lo


@log_function_call(slow=True)
def da_search_data(df: pd.DataFrame, data, start_row=None, start_col=None,
                   case_sensitive: bool = False, use_regex: bool = False, whole_cell: bool = False,
                   row_range=None, col_range=None, max_hits: int = 100000):
    """
    在DataFrame中查找匹配数据的单元格位置（002 T1.4 扩展：文本/正则/整格/范围/上限截断）

    新参数一律追加在 start_row/start_col 之后，旧位置参数调用组合行为保持不变。
    匹配语义见 _da_column_find_mask（data 为字符串时）；data 为非字符串（数值/NaN）
    时走旧等值比较路径。

    参数:
        df: 目标DataFrame
        data: 关键字（字符串走文本匹配；其它类型走旧等值语义）
        start_row: 旧参数：起始行号（等价 row_range=(start_row, None)，row_range 优先）
        start_col: 旧参数：起始列号（等价 col_range=(start_col, None)，col_range 优先）
        case_sensitive: 区分大小写
        use_regex: 正则表达式（只作用于文本类列；无效正则抛 re.error）
        whole_cell: 整格匹配
        row_range: 行范围闭区间 (lo, hi)，None 表示全表
        col_range: 列范围闭区间 (lo, hi)，None 表示全表
        max_hits: 命中上限（默认 100000，§4.2 硬约束③），超出截断返回数组

    返回:
        tuple: (rows, cols, total)
            rows/cols: numpy int32 数组（行主序，最多 max_hits 条）
            total: 真实命中总数（> len(rows) 时说明发生了截断，调用方据此提示）
    """
    if row_range is None and start_row:
        row_range = (int(start_row), None)
    if col_range is None and start_col:
        col_range = (int(start_col), None)
    if isinstance(data, str):
        if not data:
            return (np.empty(0, dtype=np.int32), np.empty(0, dtype=np.int32), 0)
        if use_regex:
            import re
            re.compile(data)  # 无效正则尽早抛 re.error（C++ 侧转用户提示，不静默）
        rows, cols = _da_find_hits(df, data, case_sensitive, use_regex, whole_cell, row_range, col_range)
    else:
        rows, cols = _da_find_hits_equality(df, data, row_range, col_range)
    total = int(rows.size)
    if max_hits is not None and max_hits >= 0 and total > max_hits:
        rows = rows[:max_hits]
        cols = cols[:max_hits]
    return (rows.astype(np.int32), cols.astype(np.int32), total)


def _da_coerce_to_dtype(dtype, text: str):
    '''
    把替换文本按列 dtype 转换（数值列整格替换保持 dtype，§4.2）；
    无法无损转换（非数值文本 / 整型列遇小数）返回 None，该列不参与替换
    '''
    num = _da_parse_numeric_keyword(text)
    if num is None:
        return None
    if dtype.kind == 'f':
        return dtype.type(num)
    if dtype.kind in ('i', 'u'):
        if not float(num).is_integer():
            return None
        return dtype.type(int(num))
    return None


@log_function_call(slow=True)
def da_find_and_replace(df: pd.DataFrame, find: str, replace: str,
                        case_sensitive: bool = False, use_regex: bool = False, whole_cell: bool = False,
                        row_range=None, col_range=None):
    """
    按 da_search_data 同一匹配语义就地替换，返回变更清单（供值记录撤销，D4）

    替换规则（§4.2 v1）：
    - 文本列：正则 → pandas str.replace(regex=True)（re.sub 语义，replace 可引用捕获组）；
      非正则 → 字面子串替换；整格匹配 → 整格替换为 replace 字面量
    - 数值列：仅整格精确替换——关键字解析为数值且整格相等时，写回按列 dtype 转换的
      同类型值（保持 dtype）；replace 无法转换到该列 dtype 时该列不替换
    - 正则模式不作用于数值列；日期/布尔等其它列 v1 不参与替换

    返回:
        tuple: (rows, cols, old_values, new_values)
            rows/cols: numpy int32 数组（行主序）
            old_values/new_values: Python list（与 rows/cols 一一对应，撤销/重做写回用）
    """
    if not isinstance(find, str) or not find:
        return (np.empty(0, dtype=np.int32), np.empty(0, dtype=np.int32), [], [])
    if use_regex:
        import re
        re.compile(find)  # 无效正则尽早抛 re.error
    r_lo, r_hi = _da_resolve_bound(row_range, len(df))
    c_lo, c_hi = _da_resolve_bound(col_range, df.shape[1])
    all_rows = []
    all_cols = []
    all_old = []
    all_new = []
    for c in range(c_lo, c_hi + 1):
        values = df.iloc[r_lo:r_hi + 1, c]
        dt = values.dtype
        if _da_is_text_column(values):
            mask = _da_column_find_mask(values, find, case_sensitive, use_regex, whole_cell)
            idx = np.flatnonzero(mask)
            if idx.size == 0:
                continue
            arr = values.to_numpy()
            if whole_cell and not use_regex:
                new_vals = [replace] * idx.size
            elif use_regex:
                replaced = values.str.replace(find, replace, case=case_sensitive, regex=True)
                new_vals = np.asarray(replaced.to_numpy(), dtype=object)[idx].tolist()
            else:
                replaced = values.str.replace(find, replace, case=case_sensitive, regex=False)
                new_vals = np.asarray(replaced.to_numpy(), dtype=object)[idx].tolist()
            old_vals = np.asarray(arr, dtype=object)[idx].tolist()
        elif pd.api.types.is_numeric_dtype(dt) and dt.kind != 'b' and not use_regex:
            # 数值列：仅整格精确替换（关键字等值命中 → 写回同类型值，保持 dtype）
            num = _da_parse_numeric_keyword(find)
            if num is None:
                continue
            new_num = _da_coerce_to_dtype(dt, replace)
            if new_num is None:
                continue  # replace 无法保持 dtype，该列不替换
            arr = values.to_numpy()
            idx = np.flatnonzero(np.asarray(arr == num, dtype=bool))
            if idx.size == 0:
                continue
            old_vals = arr[idx].tolist()
            new_vals = [new_num] * idx.size
        else:
            continue  # 日期/布尔等其它类型列 v1 不参与替换
        # 就地写回（位置赋值，避免 index 对齐；dtype 保持）
        df.iloc[idx + r_lo, c] = new_vals
        all_rows.append(idx + r_lo)
        all_cols.append(np.full(idx.size, c, dtype=np.int64))
        all_old.extend(old_vals)
        all_new.extend(new_vals)
    if not all_rows:
        return (np.empty(0, dtype=np.int32), np.empty(0, dtype=np.int32), [], [])
    rows = np.concatenate(all_rows)
    cols = np.concatenate(all_cols)
    order = np.lexsort((cols, rows))  # 行主序
    rows = rows[order]
    cols = cols[order]
    old_sorted = [all_old[i] for i in order]
    new_sorted = [all_new[i] for i in order]
    return (rows.astype(np.int32), cols.astype(np.int32), old_sorted, new_sorted)


@log_function_call(slow=True)
def da_write_cells(df: pd.DataFrame, rows, cols, values):
    """
    按 (行, 列) 位置批量写回单元格值（替换命令的撤销/重做用，D4 值记录）

    按列分组做位置赋值（df.iloc[行数组, 列号] = 值列表），避免逐格 Python 循环，
    大变更清单下也保持可用；不做 index 对齐，dtype 由 pandas 按列写回保持。

    :param df: pd.DataFrame，就地修改
    :param rows: 行位置序列
    :param cols: 列位置序列
    :param values: 值序列（与 rows/cols 一一对应）
    :return: 此函数不返回值，直接改变df
    """
    rows_arr = np.asarray(rows, dtype=np.int64)
    cols_arr = np.asarray(cols, dtype=np.int64)
    if rows_arr.size == 0:
        return
    if rows_arr.size != cols_arr.size or rows_arr.size != len(values):
        raise ValueError("rows/cols/values size mismatch")
    values = list(values)
    order = np.argsort(cols_arr, kind='stable')
    sorted_cols = cols_arr[order]
    boundaries = np.flatnonzero(np.r_[True, sorted_cols[1:] != sorted_cols[:-1]])
    ends = np.r_[boundaries[1:], rows_arr.size]
    for s, e in zip(boundaries, ends):
        idx = order[s:e]
        c = int(sorted_cols[s])
        df.iloc[rows_arr[idx], c] = [values[i] for i in idx]


@log_function_call(slow=True)
def da_fill_selection(df: pd.DataFrame, col: int, row_start: int, row_end: int, mode: str):
    """
    选区填充求值（005 T1.24，设计稿 §4.8）：在某一列的选中行区间 [row_start, row_end]
    （闭区间，绝对行位置）内，按 mode 计算填充后的值，**只返回实际发生变化的单元格**
    （行位置 + 新值），供 C++ 侧组织成一条 DACommandDataFrame_paste（值记录，O(改动量)）。

    空值判定（NaN/NaT/None）一律由 pandas 完成——C++ 侧 QVariant 对空值语义不可靠，
    这是设计稿的硬要求。本函数**不修改 df**，是纯求值。

    mode 语义（每列在自己的选中区间内独立处理，与 Excel 一致）：
    - 'down'  向下填充：取区间首行值填满其余行（首行值为空则整列填充为空，与 Excel 一致）
    - 'up'    向上填充：取区间末行值填满其余行
    - 'ffill' 向下传递最后一个非空值（区间内 pandas ffill）
    - 'bfill' 向上传递下一个非空值（区间内 pandas bfill）

    :param df: pd.DataFrame（不修改）
    :param col: 列位置索引
    :param row_start: 起始行（含，绝对位置）
    :param row_end: 结束行（含，绝对位置）
    :param mode: 'down'/'up'/'ffill'/'bfill'
    :return: (rows, values) —— rows 为 np.int32 数组（要写入的绝对行位置，升序），
             values 为等长 list（对应新值，保持 pandas 原生类型以便 iat 按列 dtype 写回）；
             区间不足两行或无变化时返回 (空数组, 空列表)
    """
    nrow = df.shape[0]
    ncol = df.shape[1]
    empty = (np.empty(0, dtype=np.int32), [])
    if col < 0 or col >= ncol:
        return empty
    row_start = max(0, int(row_start))
    row_end = min(int(nrow) - 1, int(row_end))
    if row_end - row_start < 1:
        return empty
    sub = df.iloc[row_start:row_end + 1, col]
    if mode == 'down':
        anchor = df.iat[row_start, col]
        filled = pd.Series([anchor] * len(sub), index=sub.index)
    elif mode == 'up':
        anchor = df.iat[row_end, col]
        filled = pd.Series([anchor] * len(sub), index=sub.index)
    elif mode == 'ffill':
        filled = sub.ffill()
    elif mode == 'bfill':
        filled = sub.bfill()
    else:
        return empty
    orig_vals = sub.tolist()
    filled_vals = filled.tolist()
    rows = []
    values = []
    for i in range(len(sub)):
        ov = orig_vals[i]
        fv = filled_vals[i]
        ov_na = bool(pd.isna(ov)) if not isinstance(ov, (list, tuple, dict)) else False
        fv_na = bool(pd.isna(fv)) if not isinstance(fv, (list, tuple, dict)) else False
        if ov_na and fv_na:
            continue  # 空 → 空，无变化
        if ov_na != fv_na:
            rows.append(row_start + i)
            values.append(fv)
            continue
        # 两者都非空：直接比较（NaN 已在上面排除）
        try:
            same = bool(ov == fv)
        except Exception:
            same = False
        if not same:
            rows.append(row_start + i)
            values.append(fv)
    return (np.asarray(rows, dtype=np.int32), values)


@log_function_call(slow=True)
def da_fill_series(count: int, kind: str = 'linear', start=None, step=None, stop=None, date_unit: str = 'D'):
    """
    序列填充求值（005 T1.24）：生成一个长度为 count 的序列（数值等差 / 日期步进），
    由 Python 求值（date_range/DateOffset 语义），供对话框预览与实际写入共用。
    **不接触 df**，只按参数生成值列表；每个被选中的列各调用一次得到自己的序列。

    :param count: 需要生成的值个数（= 目标列被选中的行数），<=0 返回空列表
    :param kind: 'linear' 数值等差 / 'date' 日期步进
    :param start: 起始值（linear 为数值，date 为可被 pd.Timestamp 解析的值；None 用默认）
    :param step: 步长（linear 为数值增量，date 为每个日期单位的整数倍数）
    :param stop: 终止值（仅 linear 用；沿 step 方向越过即停止，最多 count 个）
    :param date_unit: 日期单位 'D' 天 / 'W' 周 / 'M' 月 / 'Y' 年（DateOffset 语义）
    :return: list —— linear 为 float 列表，date 为 pd.Timestamp 列表
    """
    count = int(count)
    if count <= 0:
        return []
    if kind == 'date':
        base = pd.Timestamp(start) if start is not None else pd.Timestamp.today().normalize()
        step_n = int(step) if step is not None else 1
        if date_unit == 'D':
            offset = pd.Timedelta(days=step_n)
        elif date_unit == 'W':
            offset = pd.Timedelta(weeks=step_n)
        elif date_unit == 'M':
            offset = pd.DateOffset(months=step_n)
        elif date_unit == 'Y':
            offset = pd.DateOffset(years=step_n)
        else:
            offset = pd.Timedelta(days=step_n)
        return [base + i * offset for i in range(count)]
    # linear 等差
    s = float(start) if start is not None else 0.0
    st = float(step) if step is not None else 1.0
    has_stop = stop is not None and str(stop) != ''
    stop_v = float(stop) if has_stop else None
    vals = []
    cur = s
    for _ in range(count):
        if has_stop:
            if st >= 0 and cur > stop_v:
                break
            if st < 0 and cur < stop_v:
                break
        vals.append(cur)
        cur += st
    return vals


def _da_resolve_column_position(df: pd.DataFrame, name: str, occurrence: int) -> int:
    """
    列名 + 第 n 个同名列 → 列位置索引（R20 重名消歧）。occurrence 从 0 起。
    找不到返回 -1。
    """
    seen = 0
    for i, c in enumerate(df.columns):
        if str(c) == str(name):
            if seen == int(occurrence):
                return i
            seen += 1
    return -1


@log_function_call(slow=True)
def da_validate_dataframe(df: pd.DataFrame, rules):
    """
    数据校验求值（005 T1.25，设计稿 §4.9）：逐规则求违规行清单，**只读不改 df**。
    空值判定（NaN/NaT/None）与比较全部由 pandas 完成（红线1：C++ QVariant 不可靠）。

    :param df: pd.DataFrame（不修改）
    :param rules: 规则列表，每条为 dict：
        { 'index': int（规则序号，原样回传）, 'column': str（列名）,
          'occurrence': int（第 n 个同名列，0 起）, 'type': str, ...类型参数 }
        type 取值与参数（与 DATableValidationRule 求值语义钉死一致）：
        - 'numeric-range': numeric_min/numeric_max(float), has_min/has_max(bool)；
          值可转数值且越界 → 违规；空值/非数值不判（交给 not-empty）
        - 'date-range': date_min/date_max(可被 pd.Timestamp 解析的字符串或 None)；
          值可转日期且越界 → 违规；空值不判；闭区间
        - 'enum': enum_values(list)；值不在集合内 → 违规（**空值也违规**）
        - 'not-empty': 空值 → 违规
        - 'unique': 列内重复（duplicated keep=False，重复组全部行违规）
        - 'regex': regex_pattern(str)；整格完全匹配(fullmatch)不通过 → 违规；
          空值不判；正则非法抛 ValueError
    :return: list of (index, rows)：index 为规则序号，rows 为 np.int32 数组（违规行的
             **绝对行位置**，升序）；列不存在时 rows 为空数组（不报错，规则被跳过）
    """
    import re
    results = []
    for rule in rules:
        idx = int(rule.get('index', -1))
        col_pos = _da_resolve_column_position(df, rule.get('column', ''), int(rule.get('occurrence', 0)))
        if col_pos < 0:
            results.append((idx, np.empty(0, dtype=np.int32)))
            continue
        ser = df.iloc[:, col_pos]
        rtype = rule.get('type', '')
        if rtype == 'numeric-range':
            num = pd.to_numeric(ser, errors='coerce')
            viol = pd.Series(False, index=ser.index)
            if rule.get('has_min'):
                viol = viol | (num < float(rule['numeric_min']))
            if rule.get('has_max'):
                viol = viol | (num > float(rule['numeric_max']))
            # to_numeric 对空值/非数值产出 NaN，比较结果为 False，天然「不判」
        elif rtype == 'date-range':
            dt = pd.to_datetime(ser, errors='coerce')
            viol = pd.Series(False, index=ser.index)
            dmin = rule.get('date_min')
            dmax = rule.get('date_max')
            if dmin is not None and str(dmin) != '':
                viol = viol | (dt < pd.Timestamp(dmin))
            if dmax is not None and str(dmax) != '':
                viol = viol | (dt > pd.Timestamp(dmax))
        elif rtype == 'enum':
            allowed = list(rule.get('enum_values', []))
            viol = ~ser.isin(allowed)  # 空值(NaN/None)不在集合内 → 违规
        elif rtype == 'not-empty':
            viol = ser.isna()
        elif rtype == 'unique':
            viol = ser.duplicated(keep=False)
        elif rtype == 'regex':
            pattern = rule.get('regex_pattern', '')
            compiled = re.compile(pattern)  # 非法正则抛 re.error（调用方转违规提示）
            notna = ser.notna()
            matched = ser.astype('string').map(lambda x: compiled.fullmatch(x) is not None if pd.notna(x) else True)
            viol = notna & (~matched.fillna(True))
        else:
            results.append((idx, np.empty(0, dtype=np.int32)))
            continue
        rows = np.flatnonzero(viol.to_numpy(dtype=bool)).astype(np.int32)
        results.append((idx, rows))
    return results


@log_function_call(slow=True)
def da_data_select(df: pd.DataFrame, index: str, lower: Optional[float] = None, upper: Optional[float] = None):
    '''
    :param df: pd.DataFrame
    :param lower: 范围下界，None表示无下限
    :param upper: 范围上界，None表示无上限
    :param index: 选中的列索引
    '''
    # 参数校验
    if lower is None and upper is None:
        raise ValueError("Must specify at least one of lower or upper")

    # 创建条件掩码（初始为全True）
    mask = pd.Series(True, index=df.index)

    # 在选中的列上应用范围条件
    col_series = df[index]
    if lower is not None and upper is not None:
        mask &= col_series.between(lower, upper)
    elif lower is not None:
        mask &= (col_series >= lower)
    elif upper is not None:
        mask &= (col_series <= upper)

    # 直接删除不符合条件的行
    df.drop(index=df[~mask].index, inplace=True)


def _column_position(df: pd.DataFrame, col):
    '''
    列位置或列名 → 列位置。重名列取第一个匹配（与视图状态存储的消歧约定一致）
    :param df: pd.DataFrame
    :param col: 列位置(int)或列名(str)
    :return: 列位置(int)
    :raises ValueError: 越界或列不存在
    '''
    if isinstance(col, (int, np.integer)):
        pos = int(col)
        if pos < 0 or pos >= df.shape[1]:
            raise ValueError("column position %d out of range" % pos)
        return pos
    matches = [i for i, c in enumerate(df.columns) if c == col or str(c) == str(col)]
    if not matches:
        raise ValueError("column %r not found" % (col,))
    return matches[0]


def _resolve_sort_keys(df: pd.DataFrame, keys, by, ascending: bool):
    '''
    归一化排序键为 [(列位置, 升序bool)]；keys 优先于 by/ascending
    '''
    resolved = []
    if keys is not None:
        for item in keys:
            col, asc = item[0], bool(item[1])
            resolved.append((_column_position(df, col), asc))
    elif by is not None:
        resolved.append((_column_position(df, by), bool(ascending)))
    else:
        raise ValueError("da_sort requires either keys or by")
    if not resolved:
        raise ValueError("da_sort requires at least one sort key")
    return resolved


def _stable_descending_argsort(values: np.ndarray) -> np.ndarray:
    '''
    降序稳定排序：等值元素保持原相对顺序（升序稳定排序 + 整表反转 + 等值段内再反转）。
    不能直接用 argsort 反转：那会把等值元素的相对顺序也颠倒，破坏多键排序的次键结果
    :param values: 一维数组（调用方保证无 NaN/NaT）
    :return: 置换数组 order（new[i] = old[order[i]]）
    :raises TypeError: 值不可比较（混合类型 object 列）
    '''
    if values.dtype.kind == 'f':
        # 浮点快速路径：取负后升序稳定排序（调用方已剔除 NaN）
        return np.argsort(-values, kind='stable')
    asc_order = np.argsort(values, kind='stable')
    rev = asc_order[::-1]
    n = rev.shape[0]
    if n <= 1:
        return rev
    sorted_vals = values[rev]
    equal = np.zeros(n, dtype=bool)
    try:
        equal[1:] = (sorted_vals[1:] == sorted_vals[:-1])
    except TypeError:
        # object 数组逐元素比较（混合类型在此抛 TypeError，由调用方包装为 ValueError）
        for i in range(1, n):
            equal[i] = bool(sorted_vals[i] == sorted_vals[i - 1])
    # 整表反转后，等值段内部再反转一次，恢复等值元素的原始相对顺序
    result = rev.copy()
    i = 0
    while i < n:
        j = i + 1
        while j < n and equal[j]:
            j += 1
        if j - i > 1:
            result[i:j] = rev[i:j][::-1]
        i = j
    return result


def _single_key_permutation(values: np.ndarray, ascending: bool) -> np.ndarray:
    '''
    单键稳定排序置换；NaN/NaT 恒排最后（与 pandas na_position='last' 一致，
    也与既有 sort_values 行为的用户文档口径一致）
    :param values: 一维数组（当前排列下的键值）
    :param ascending: 是否升序
    :return: 置换数组（作用于传入的 values 序列）
    :raises ValueError: 值类型混合不可比较（消息含 "not comparable"）
    '''
    n = values.shape[0]
    mask = pd.isna(values)
    try:
        if mask.all():
            return np.arange(n)
        if mask.any():
            valid_idx = np.flatnonzero(~mask)
            na_idx = np.flatnonzero(mask)
            valid_values = values[valid_idx]
            order = np.argsort(valid_values, kind='stable') if ascending else _stable_descending_argsort(valid_values)
            return np.concatenate([valid_idx[order], na_idx])
        return np.argsort(values, kind='stable') if ascending else _stable_descending_argsort(values)
    except TypeError as e:
        raise ValueError("column values are not comparable: %s" % (e,))


def _apply_row_permutation(df: pd.DataFrame, perm):
    '''
    按置换就地重排行：new[i] = old[perm[i]]，index 随行一起移动，各列 dtype 保持不变。
    逐列位置写回（df.iloc[:, i] = ...），重名列也不受影响
    :param df: pd.DataFrame，就地修改
    :param perm: 置换序列（长度必须等于行数；空置换视为无操作）
    '''
    perm_arr = np.asarray(perm, dtype=np.int64)
    if perm_arr.size == 0:
        return
    if perm_arr.size != len(df):
        raise ValueError("permutation size %d does not match row count %d" % (perm_arr.size, len(df)))
    sorted_df = df.iloc[perm_arr]
    for i in range(df.shape[1]):
        df.iloc[:, i] = sorted_df.iloc[:, i].to_numpy()
    df.index = sorted_df.index


@log_function_call(slow=True)
def da_sort(df: pd.DataFrame, keys=None, by=None, ascending: bool = True):
    '''
    多键稳定排序，就地重排 df 并返回置换 perm

    perm[i] = 新表第 i 行在旧表中的行号（新→旧，与 df.iloc[perm] 语义一致，
    同 C++ DATableKeyRemap / DATableStyleManager::onRowsReordered 的置换约定）。
    多键按 keys 列表顺序为优先级（第一个是主键），通过"从次键到主键逐键稳定排序"
    实现；NaN/NaT 恒排最后。

    :param df: pd.DataFrame，就地重排（行数据与 index 一起移动）
    :param keys: [(列位置int或列名str, ascending bool)] 列表，优先于 by/ascending
    :param by: 单键兼容入口：列位置或列名
    :param ascending: 单键兼容入口：是否升序
    :return: perm（Python int 列表）；空表返回空列表
    :raises ValueError: 键列不存在/越界，或列值类型混合不可比较（消息含 "not comparable"）
    '''
    resolved = _resolve_sort_keys(df, keys, by, ascending)
    n = len(df)
    if n == 0:
        return []
    perm = np.arange(n)
    # 从最后一个键（最低优先级）到第一个键（主键）逐键稳定排序
    for pos, asc in reversed(resolved):
        col_values = df.iloc[:, pos].to_numpy()
        order = _single_key_permutation(col_values[perm], asc)
        perm = perm[order]
    _apply_row_permutation(df, perm)
    return perm.tolist()


@log_function_call(slow=True)
def da_reorder_rows(df: pd.DataFrame, perm):
    '''
    按给定置换就地重排行：new[i] = old[perm[i]]（与 da_sort 返回的 perm 同语义）。
    排序命令的 redo/undo 用它施加正置换/逆置换，不重新计算排序

    :param df: pd.DataFrame，就地重排
    :param perm: 置换序列（长度必须等于行数）
    :return: 此函数不返回值，直接改变df
    '''
    _apply_row_permutation(df, perm)


@log_function_call(slow=True)
def da_column_stats(df: pd.DataFrame, column=None, cols=None, row_range=None, unique_limit=0):
    '''
    列统计：兼容路径返回 (min, max, median)；选区路径返回 list[dict]（每列全部指标）

    :param column: 旧路径的列位置索引（int）；仅 cols is None 时使用
    :param cols: 新区间路径的列位置索引列表；None = 走旧路径（003 语义逐条不变）
    :param row_range: (begin, end) 半开区间（数据行号）；None = 全表
    :param unique_limit: 非数值列行数 ≤ 此值时才给唯一值数；0 = 不要
    :return: 旧路径 (min, max, median) 或 None；新区间路径 list[dict]
    '''
    if cols is None:                      # ===== 003 兼容路径：逐条不变 =====
        if column < 0 or column >= df.shape[1]:
            raise IndexError("column index %d out of range [0, %d)" % (column, df.shape[1]))
        numeric = pd.to_numeric(df.iloc[:, column], errors='coerce')
        valid = numeric.dropna()
        if valid.size == 0:
            return None
        return (float(valid.min()), float(valid.max()), float(valid.median()))
    # ===== 新区间路径（007 T1.7）=====
    sub = df if row_range is None else df.iloc[int(row_range[0]):int(row_range[1])]   # 半开区间；切片，不复制整表
    rows = int(sub.shape[0])
    out = []
    for c in cols:
        series = sub.iloc[:, int(c)]                  # 位置索引
        count = int(series.count())                   # 非空计数（NaN/NaT/None 一律算空）
        null_count = int(series.isna().sum())
        is_num = pd.api.types.is_numeric_dtype(series) and not pd.api.types.is_bool_dtype(series)
        unique_count = None
        if unique_limit and rows <= int(unique_limit) and not is_num:   # 数值列不请求 nunique（省最贵的规约）
            unique_count = int(series.nunique(dropna=True))
        stats = {"count": count, "null_count": null_count, "unique_count": unique_count,
                 "sum": None, "mean": None, "min": None, "max": None}
        if is_num and count > 0:                      # 全空（count == 0）→ 四项全 None（同 003 口径）
            stats["sum"] = float(series.sum())
            stats["mean"] = float(series.mean())
            stats["min"] = float(series.min())
            stats["max"] = float(series.max())
        out.append(stats)
    return out


@log_function_call(slow=True)
def da_to_csv(df: pd.DataFrame, path: str, sep: str):
    '''
    把dataframe写到文件

    注意：此函数与 io.da_to_csv 签名不同。io.da_to_csv 接收 args 字典参数，
    此函数接收 sep 参数并固定 index=False。两者均为 da 系统默认函数，
    由 C++ 侧按函数名调用，不可重命名或删除。

    :param df: pd.DataFrame
    :param path: 文件路径
    :param sep: 分隔符
    :return: 此函数不返回值
    '''
    df.to_csv(path, sep=sep, index=False)


@log_function_call(slow=True)
def da_to_excel(df: pd.DataFrame, path: str):
    '''
    把dataframe写到文件

    注意：此函数与 io.da_to_excel 签名不同。io.da_to_excel 接收 args 字典参数，
    此函数固定 index=False。两者均为 da 系统默认函数，不可重命名或删除。

    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值
    '''
    df.to_excel(path, index=False)


@log_function_call(slow=True)
def da_to_pickle(df: pd.DataFrame, path: str):
    '''
    把dataframe写到文件

    注意：此函数与 io.da_to_pickle 签名不同。io.da_to_pickle 接收 args 字典参数，
    此函数不接收额外参数。两者均为 da 系统默认函数，不可重命名或删除。

    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值
    '''
    df.to_pickle(path)


@log_function_call(slow=True)
def da_to_parquet(df: pd.DataFrame, path: str):
    '''
    把dataframe写到文件

    注意：此函数与 io.da_to_parquet 签名不同。io.da_to_parquet 接收 args 字典参数，
    此函数不接收额外参数。两者均为 da 系统默认函数，不可重命名或删除。

    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值
    '''
    df.to_parquet(path)


@log_function_call(slow=True)
def da_from_pickle(df: pd.DataFrame, path: str):
    '''
    从文件加载到dataframe

    原地更新 df 的内容（C++ 侧持有 df 引用，要求原地修改）。
    不使用 df.__init__() 替换内容，而是先清空再填充。

    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值，直接改变df
    '''
    tmp = pd.read_pickle(path)
    # 清空原 df（先删列再删行，使 df 完全空）
    df.drop(columns=df.columns.tolist(), inplace=True)
    df.drop(df.index, inplace=True)
    # 填入新数据（赋值 Series 会自动设置索引）
    for col in tmp.columns:
        df[col] = tmp[col]


@log_function_call(slow=True)
def da_from_parquet(df: pd.DataFrame, path: str):
    '''
    从文件加载到dataframe

    原地更新 df 的内容（C++ 侧持有 df 引用，要求原地修改）。
    不使用 df.__init__() 替换内容，而是先清空再填充。

    :param df: pd.DataFrame
    :param path: 文件路径
    :return: 此函数不返回值，直接改变df
    '''
    tmp = pd.read_parquet(path)
    # 清空原 df（先删列再删行，使 df 完全空）
    df.drop(columns=df.columns.tolist(), inplace=True)
    df.drop(df.index, inplace=True)
    # 填入新数据（赋值 Series 会自动设置索引）
    for col in tmp.columns:
        df[col] = tmp[col]


@log_function_call(slow=True)
def da_astype(df: pd.DataFrame, colsIndex: List[int], dtype: np.dtype):
    '''
    通过列索引改变dataframe的数据类型
    :param df: pd.DataFrame
    :param colsIndex: 列索引
    :param dtype: 类型
    :return: 此函数不返回值，直接改变df
    '''
    cols = [df.columns[v] for v in colsIndex]
    df[cols] = df[cols].astype(dtype)


@log_function_call(slow=True)
def da_setnan(df: pd.DataFrame, rowindex: List[int], colindex: List[int]):
    '''
    设置nan值
    :param df: pd.DataFrame
    :param rowindex: 行索引
    :param colindex: 列索引
    :return: 此函数不返回值，直接改变df
    '''
    if len(rowindex) != len(colindex):
        raise ValueError('row index length not equal column index length')
    if not rowindex:
        return
    # 按列分组，每列使用 numpy 高级索引一次性赋值
    # df[col_name].values 返回该列底层数据的视图，修改会反映到 df
    col_to_rows = {}
    for r, c in zip(rowindex, colindex):
        col_to_rows.setdefault(c, []).append(r)
    for c, rows in col_to_rows.items():
        col_name = df.columns[c]
        df[col_name].values[rows] = np.nan


@log_function_call(slow=True)
def da_cast_to_num(df: pd.DataFrame,
                   colsIndex: List[int],
                   errors: str = 'coerce',
                   downcast: Optional[str] = None):
    '''
    转换为数值
        errors = {'ignore', 'raise', 'coerce'},
    '''
    cols = [df.columns[v] for v in colsIndex]
    for col in cols:
        df[col] = pd.to_numeric(
            df[col], errors=errors, downcast=downcast)


@log_function_call(slow=True)
def da_cast_to_datetime(df: pd.DataFrame,
                        colsIndex: List[int],
                        errors: str = 'coerce',
                        dayfirst: bool = False,
                        yearfirst: bool = False,
                        utc: Optional[bool] = None,
                        format: Optional[str] = None,
                        exact: bool = True,
                        unit: Optional[str] = 'ns',
                        infer_datetime_format: bool = False,
                        origin: str = 'unix',
                        cache: bool = True
                        ):
    '''
    转换为日期
        errors = {'ignore', 'raise', 'coerce'},
        format
    '''
    cols = [df.columns[v] for v in colsIndex]
    for col in cols:
        df[col] = pd.to_datetime(
            df[col], errors=errors, dayfirst=dayfirst, yearfirst=yearfirst, utc=utc,
            format=format, exact=exact,
            unit=unit, infer_datetime_format=infer_datetime_format,
            origin=origin, cache=cache)


@log_function_call(slow=True)
def da_setindex(df: pd.DataFrame, colsIndex: List[int]):
    '''
    设置索引
    :param df:
    :param colsIndex:
    :return:
    '''
    cols = [df.columns[v] for v in colsIndex]
    index_ser = df.index
    # 这时要把index设置到列里面
    if len(index_ser.names) > 1:
        # 多重索引
        dfindex = index_ser.to_frame()
        for name in index_ser.names:
            cnt = 1
            cn = name
            while cn in df.columns:
                cn = f'{name}_{cnt}'
                cnt += 1
            df[cn] = dfindex[name]
    else:
        cnt = 1
        while index_ser.name in df.columns:
            index_ser.name = f'{index_ser.name}_{cnt}'
            cnt += 1
        df[index_ser.name] = index_ser
    # 把列转换为index
    df.set_index(cols, inplace=True)


@log_function_call(slow=True)
def da_insert_nanrow(df: pd.DataFrame, row: int):
    '''
    插入一行，插入的行默认为nan
    :param df:
    :param row:
    :return:
    '''
    dfindex = df.index
    cols = df.columns
    df1 = df.iloc[:row]
    df2 = df.iloc[row:]
    dfnanrow = pd.DataFrame(df.iloc[row, :]).T
    # pandas 3 起 .at 只接受标量标签（.at[:, :] 抛 InvalidIndexError），改用 .iloc 广播赋 NaN
    dfnanrow.iloc[:, :] = np.nan
    if isinstance(dfindex, pd.RangeIndex):
        df.__init__(pd.concat([df1, dfnanrow, df2], ignore_index=True))
    else:
        df.__init__(pd.concat([df1, dfnanrow, df2], ignore_index=False))


@log_function_call(slow=True)
def da_insert_column(df: pd.DataFrame, col: int, name: str,
                     dtype: Optional[np.dtype] = None,
                     defaultvalue=np.nan,
                     start=None, stop=None):
    '''插入一列，插入的列默认为nan

    Args:
        df (pd.DataFrame): 数据
        col (int): 列号
        name (str): 列名
        dtype (np.dtype, optional):dtype,对于日期，明确dtype才能正确处理. Defaults to None.
        defaultvalue ([type], optional): 默认值，如果设置stop将不起作用. Defaults to np.nan.
        start ([type], optional): 使用np.arange/np.linspace生成列，必须和stop搭配 Defaults to None.
        stop ([type], optional): [description]. Defaults to None.
    '''
    if start is None:
        df.insert(col, name, np.full(df.shape[0], defaultvalue, dtype=dtype))
    else:
        s = None
        # 对于数值，使用linspace，对于日期使用arange
        if dtype == np.datetime64:
            # 日期单独处理
            if stop is not None:
                # 指定了结束日期
                s = pd.date_range(start, stop, periods=df.shape[0])
            else:
                # 没有指定结束日期，就每秒递增
                start = np.datetime64(start)
                step = np.timedelta64(1, 's')
                stop = start + step*df.shape[0]
                s = np.arange(start, stop, step)
        else:
            if stop is not None:
                s = np.linspace(start, stop, df.shape[0])
            else:
                s = np.linspace(start, start+df.shape[0], df.shape[0])
        df.insert(col, name, s)


@log_function_call(slow=True)
def da_itake_column(df: pd.DataFrame, col: int) -> pd.Series:
    '''提取一列

    Args:
        df (pd.DataFrame): DataFrame
        col (int): 列索引
    '''
    s = df[df.columns[col]]
    da_drop_icolumn(df=df, index=[col])
    return s


@log_function_call(slow=True)
def da_insert_at(df: pd.DataFrame, col: int, series: pd.Series):
    df.insert(col, series.name, series)


def make_dataframe(size: int = 100) -> pd.DataFrame:
    '''构建一个数据类型较全面的dataframe

    Args:
        size (int, optional): [description]. Defaults to 100.

    Returns:
        (pd.DataFrame): [description]
    '''
    df = pd.DataFrame()
    dt = np.timedelta64(1, 's')
    df['datatime'] = np.arange(np.datetime64(
        '2021-01-01 00'), np.datetime64('2021-01-01 00')+dt*size, dt, dtype=np.datetime64)
    df['int64'] = np.linspace(-10, 10, size, dtype=np.int64)
    df['uint64'] = np.linspace(0, size, size, dtype=np.uint64)
    df['float32'] = np.linspace(0, size, size, dtype=np.float32)
    df['float32'] = np.sin(df['float32'])
    df['float64'] = np.linspace(0, size, size, dtype=np.float64)
    df['float64'] = np.cos(df['float64'])
    df['str'] = np.array(['text']*size, dtype=np.str_)
    df['complex128'] = np.fft.fft(df['float64'])
    return df


def __tst_pickle():
    df = pd.read_csv(r'F:\work\[07]数据挖掘\查询power.csv')
    path = f'./{id(df)}.pkl'
    da_to_pickle(df, path)
    print('df id =', id(df))
    index = [1, 2, 3]
    print(df)
    bed = da_drop_irow(df, [1, 2, 3])
    da_from_pickle(df, path)
    print(df)
    print('df id =', id(df))


def __tst_drop():
    df = pd.read_csv(r'F:\work\[07]数据挖掘\查询power.csv')
    print(df)
    print(df.columns)
    da_drop_icolumn(df, [1, 2, 3])
    print(df.columns)
    da_drop_irow(df, [1, 2, 3])
    print(df)


def __tst_astype():
    df = pd.read_csv(r'F:\work\[07]数据挖掘\查询power.csv')
    print(df.dtypes)
    da_astype(df, [5, 6, 7, 8], "float32")
    print(df.dtypes)


def __tst_setnan():
    df = pd.read_csv(r'F:\work\[07]数据挖掘\查询power.csv')
    print(df)
    da_setnan(df, [0, 1, 2, 3], [0, 1, 2, 3])
    print(df)


def __tst_setnan():
    df = pd.DataFrame(np.arange(16).reshape((4, 4)), index=[
                      'a', 'b', 'c', 'd'], columns=['1', '2', '3', '4'])
    print(df)
    da_setnan(df, [0, 1, 2, 3], [0, 1, 2, 3])
    print(df)


def __tst_insert_nanrow():
    df = make_dataframe()
    print(df.dtypes)
    da_insert_nanrow(df, 1)
    print(df.dtypes)
    print(df)

    df = make_dataframe()
    print(df.dtypes)
    da_insert_nanrow(df, 1)
    print(df.dtypes)
    print(df)


def __tst_insert_column():
    df = make_dataframe(7)
    print(df)
    da_insert_column(df=df, col=1, name='insert-nan')
    da_insert_column(df=df, col=2, name='insert-samevalue', defaultvalue=1.0)
    da_insert_column(df=df, col=3, name='insert-datetime',
                     dtype=np.datetime64, start='2020-01-01')
    da_insert_column(df=df, col=3, name='insert-datetime2',
                     dtype=np.datetime64, start='2020-01-01', stop='2021-01-01')
    print(df)


def __tst_fill_na():
    df = pd.DataFrame([[np.nan, 2, np.nan, 0],
                       [3, 4, np.nan, 1],
                       [np.nan, np.nan, np.nan, np.nan],
                       [np.nan, 3, np.nan, 4]],
                      columns=list("ABCD"))
    print(df)
    values = {"A": 0, "B": 1, "C": 2, "D": 3}
    da_fill_na(df, value=values)
    print(df)


if __name__ == '__main__':
    __tst_fill_na()
