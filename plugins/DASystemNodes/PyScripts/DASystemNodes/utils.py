# -*- coding: utf-8 -*-
"""DASystemNodes 节点通用工具函数"""


def data_to_text(value, max_lines: int = 8, max_width: int = 40) -> list[str]:
    """
    将任意数据对象转换为用于节点显示的文本行列表。

    :param value: 任意输入数据
    :param max_lines: 最大行数
    :param max_width: 每行最大字符数（按字符数截断）
    :return: 文本行列表
    """
    if value is None:
        return ["None"]

    try:
        text = str(value)
    except Exception:
        text = "<unprintable>"

    lines = text.splitlines()
    result = []
    for line in lines:
        # 按字符数截断，避免单行过长
        while len(line) > max_width:
            result.append(line[:max_width])
            line = line[max_width:]
            if len(result) >= max_lines:
                break
        if result:
            if len(result) >= max_lines:
                break
        result.append(line)
        if len(result) >= max_lines:
            break

    if len(result) > max_lines:
        result = result[:max_lines]

    return result if result else [""]
