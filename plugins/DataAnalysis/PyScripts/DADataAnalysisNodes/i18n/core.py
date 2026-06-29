# -*- coding: utf-8 -*-
# i18n/core.py
import gettext
import os, sys
import locale
from typing import Optional

# 翻译文件根目录（定位到 i18n/locale）
LOCALES_DIR = os.path.join(os.path.dirname(__file__), "locale")
# 库的翻译域（用包名，避免和其他库冲突）
DOMAIN = "DADataAnalysisNodes"


def get_system_language() -> str:
    """
    获取系统默认语言
    优先级：环境变量 > 系统locale > 默认值
    """
    env_lang = os.environ.get('LANG') or os.environ.get('LC_ALL') or os.environ.get('LC_MESSAGES')
    if env_lang:
        lang_code = env_lang.split('.')[0]
        return _normalize_language_code(lang_code)

    try:
        sys_lang, _ = locale.getdefaultlocale()
        if sys_lang:
            return _normalize_language_code(sys_lang)
    except:
        pass

    if sys.platform == "win32":
        try:
            import ctypes
            windll = ctypes.windll.kernel32
            lang_id = windll.GetUserDefaultUILanguage()
            win_lang_map = {
                0x0409: "en_US",
                0x0804: "zh_CN",
                0x0404: "zh_TW",
                0x0411: "ja_JP",
                0x0407: "de_DE",
                0x040C: "fr_FR",
                0x0410: "it_IT",
                0x0C0A: "es_ES",
                0x0412: "ko_KR",
                0x0419: "ru_RU",
            }
            return win_lang_map.get(lang_id, "en_US")
        except:
            pass

    return "zh_CN"


def _normalize_language_code(lang_code: str) -> str:
    lang_code = lang_code.split('.')[0]
    lang_code = lang_code.replace('-', '_')
    return lang_code


def get_available_languages() -> list:
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
