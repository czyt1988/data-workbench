# -*- coding: utf-8 -*-
# i18n/core.py
import gettext
import os, sys
import locale
from typing import Optional

# 翻译文件根目录（定位到 i18n/locale）
LOCALES_DIR = os.path.join(os.path.dirname(__file__), "locale")
# 库的翻译域（用包名，避免和其他库冲突）
DOMAIN = "DAWorkbench"


def get_system_language() -> str:
    """
    获取系统默认语言
    优先级：环境变量 > 系统locale > 默认值
    """
    # 1. 检查环境变量（最高优先级）
    env_lang = os.environ.get('LANG') or os.environ.get('LC_ALL') or os.environ.get('LC_MESSAGES')
    if env_lang:
        lang_code = env_lang.split('.')[0]
        return _normalize_language_code(lang_code)

    # 2. 使用locale模块获取系统语言
    try:
        sys_lang, _ = locale.getdefaultlocale()
        if sys_lang:
            return _normalize_language_code(sys_lang)
    except:
        pass

    # 3. 最后尝试获取平台特定的语言
    if sys.platform == "win32":
        try:
            import ctypes
            windll = ctypes.windll.kernel32
            lang_id = windll.GetUserDefaultUILanguage()
            win_lang_map = {
                0x0409: "en_US",  # 英语(美国)
                0x0804: "zh_CN",  # 简体中文
                0x0404: "zh_TW",  # 繁体中文
                0x0411: "ja_JP",  # 日语
                0x0407: "de_DE",  # 德语
                0x040C: "fr_FR",  # 法语
                0x0410: "it_IT",  # 意大利语
                0x0C0A: "es_ES",  # 西班牙语
                0x0412: "ko_KR",  # 韩语
                0x0419: "ru_RU",  # 俄语
            }
            return win_lang_map.get(lang_id, "en_US")
        except:
            pass

    # 4. 默认值
    return "zh_CN"


def _normalize_language_code(lang_code: str) -> str:
    """
    标准化语言代码
    例如：zh_CN.UTF-8 -> zh_CN, en-US -> en_US
    """
    lang_code = lang_code.split('.')[0]
    lang_code = lang_code.replace('-', '_')
    return lang_code


def get_available_languages() -> list:
    """
    获取可用的语言列表
    返回已经存在翻译文件的语言代码列表
    """
    available = []
    if os.path.exists(LOCALES_DIR):
        for lang_dir in os.listdir(LOCALES_DIR):
            lang_path = os.path.join(LOCALES_DIR, lang_dir, "LC_MESSAGES", f"{DOMAIN}.mo")
            if os.path.exists(lang_path):
                available.append(lang_dir)
    return available


def setup_i18n(
    language: Optional[str] = None,
    install_global: bool = True,
    use_system_language: bool = True
) -> gettext.GNUTranslations:
    """
    初始化库的国际化配置

    Args:
        language: 指定语言代码（如 "zh_CN", "en_US"）
        install_global: 是否安装到全局命名空间
        use_system_language: 当 language 为 None 时，是否使用系统语言

    Returns:
        翻译对象（可用于局部调用）
    """
    if language is None and use_system_language:
        language = get_system_language()
    elif language is None:
        language = "zh_CN"

    language = _normalize_language_code(language)

    available_langs = get_available_languages()
    if language not in available_langs:
        main_lang = language.split('_')[0]
        fallback_lang = None
        for lang in available_langs:
            if lang.startswith(main_lang):
                fallback_lang = lang
                break

        if fallback_lang:
            print(f"Warning: translation file for '{language}' not found, using fallback '{fallback_lang}'")
            language = fallback_lang
        else:
            print(f"Warning: translation file for '{language}' not found, using default 'zh_CN'")
            language = "zh_CN"

    try:
        trans = gettext.translation(
            domain=DOMAIN,
            localedir=LOCALES_DIR,
            languages=[language],
            fallback=True
        )
        print(f"Loaded language: {language}")
    except FileNotFoundError:
        print(f"Error: cannot load translation file for '{language}'")
        trans = gettext.NullTranslations()

    if install_global:
        trans.install()

    return trans
