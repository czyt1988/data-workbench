import re
import unicodedata
from pathlib import Path

def to_valid_name(text: str,
                  max_len: int = 200,
                  empty_default: str = 'unnamed') -> str:
    """
    将任意字符串转换成可用于保存文件的合法文件名（跨平台）。
    返回字符串保证：
      - 不含非法字符
      - 不为系统保留名
      - 不以空格/点结尾
      - 长度 <= max_len
    """
    if not isinstance(text, str):
        text = str(text)

    # 1. 正规化 Unicode（可选，防止组合字符问题）
    text = unicodedata.normalize('NFKC', text)

    # 2. 去掉首尾空格与点号
    text = text.strip(' .')

    # 3. 替换非法字符：控制字符 + < > : " / \\ | ? *
    illegal = re.compile(r'[\x00-\x1f<>:\"/\\|?*]')
    name = illegal.sub('_', text)

    # 4. 合并连续下划线，避免名字过长且难看
    name = re.sub(r'_+', '_', name)

    # 5. 若为空，则用默认
    if not name:
        name = empty_default

    # 6. 防止系统保留名（Windows 不区分大小写）
    reserved = {
        'CON', 'PRN', 'AUX', 'NUL',
        'COM1', 'COM2', 'COM3', 'COM4', 'COM5', 'COM6', 'COM7', 'COM8', 'COM9',
        'LPT1', 'LPT2', 'LPT3', 'LPT4', 'LPT5', 'LPT6', 'LPT7', 'LPT8', 'LPT9'
    }
    if name.split('.')[0].upper() in reserved:
        name += '_file'

    # 7. 去掉尾部空格/点号（再次保险）
    name = name.rstrip(' .')

    # 8. 截断长度
    if len(name) > max_len:
        # 优先保留扩展名
        p = Path(name)
        ext = ''.join(p.suffixes)          # 含多个后缀如 .tar.gz
        stem = name[:-len(ext)] if ext else name
        # 给扩展名留空间
        avail = max_len - len(ext)
        if avail <= 0:
            # 扩展名本身超长，只能硬截
            name = name[:max_len]
        else:
            name = stem[:avail] + ext

    return name


def add_counter(name: str, counter: int) -> str:
    """
    在文件名（不含扩展名部分）后追加计数器，如
    add_counter('report.pdf', 2) -> 'report (2).pdf'
    """
    if counter <= 1:
        return name
    p = Path(name)
    stem = p.stem
    ext = ''.join(p.suffixes)
    return f"{stem} ({counter}){ext}"


# ----------------- 使用示例 -----------------
if __name__ == "__main__":
    tests = [
        "月度/财务:报表*",
        "CON.txt",
        "  空格  ",
        "a" * 300 + ".docx",
        "logo.svg",
    ]
    for t in tests:
        safe = to_valid_name(t)
        print(f"原: {t!r:35} -> 合法: {safe!r}")

    # 需要重名时加计数器
    base = to_valid_name("年报*2025?.pptx")
    for i in range(1, 4):
        print(add_counter(base, i))