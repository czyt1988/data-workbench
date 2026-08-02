# DataAnalysis 插件开发指南

DAWorkbench 数据分析插件，提供基于 pandas 的数据清洗、筛选、统计、变换等 GUI 功能和工作流节点。本插件是 **C++ + Python 混合插件**的参考实现，包含完整的 i18n 基础设施。

> 📖 阅读根目录 `AGENTS.md` § 国际化（i18n）规范 了解项目级 i18n 要求；本文件聚焦 DataAnalysis 插件特有的 i18n 实践。

---

## 一、插件结构

```
DataAnalysis/
├── CMakeLists.txt              # CMake 构建（含 Python 包安装）
├── DataAnalysisPlugin.cpp/h    # C++ 插件入口（DAAbstractPlugin 实现）
├── DataAnalysisUI.cpp/h        # Ribbon UI 布局 + retranslateUi()
├── DataAnalysisBaseWorker.*    # Worker 基类
├── DataframeCleanerWorker.*    # 数据清洗 Worker（调用 Python）
├── DataframeIOWorker.*         # 数据 IO Worker
├── DataframeOperateWorker.*    # 数据操作 Worker
├── Commands.cpp/h              # QUndoCommand 子类
├── Dialogs/                    # 对话框（.ui 文件）
├── icon/                       # 图标资源
├── DataAnalysisResource.qrc    # Qt 资源文件
└── PyScripts/                  # Python 包（三层架构）
    ├── DADataAnalysisCore/     # 纯算法层（pandas/numpy，无 i18n）
    ├── DADataAnalysisGui/      # GUI 交互层（i18n 标杆）
    │   ├── __init__.py         # 调用 setup_i18n()
    │   ├── dataframe_cleaner.py
    │   ├── dataframe_io.py
    │   ├── utils.py
    │   ├── i18n/               # ⭐ Python i18n 标杆实现
    │   │   ├── core.py         # setup_i18n 核心
    │   │   ├── update_po.py    # 从 # cn: 注释自动填充 .po
    │   │   └── locale/         # .pot/.po/.mo 文件
    │   └── update-i18n.sh      # 一键生成翻译文件
    └── DADataAnalysisNodes/     # 工作流节点定义（22 节点）
        ├── __init__.py
        ├── setup.py
        └── *_node.py           # 节点定义文件
```

### Python 三层架构

| 层 | 包名 | 职责 | i18n |
|----|------|------|------|
| 算法层 | `DADataAnalysisCore` | 纯 pandas/numpy 算法（cleaning.py, io.py, operations.py） | ❌ 无（纯算法，不产生用户可见文本） |
| GUI 层 | `DADataAnalysisGui` | GUI 交互逻辑（dataframe_cleaner.py, dataframe_io.py, utils.py） | ✅ 完整 i18n（标杆） |
| 节点层 | `DADataAnalysisNodes` | @NodeDef 工作流节点定义 | ✅ 已实现 i18n |

---

## 二、i18n 实践

### C++ 部分

**模式**：`tr("English")  //cn:中文`，源文本统一英文。

- 所有 UI 文本用 `tr()` 包裹（88 处，涵盖 Ribbon 面板、Action、对话框、撤销命令名等）
- `DataAnalysisPlugin::retranslate()` 调用 `m_ui->retranslateUi()`
- `DataAnalysisUI::retranslateUi()` 集中翻译所有 Ribbon Panel 的 Action 文本和工具提示（`DataAnalysisUI.cpp` 第 136-205 行，**C++ retranslate 标杆**）
- `.ui` 文件全部使用英文，无中文硬编码
- Worker 类（如 `DataframeCleanerWorker`）只传递函数名给 Python，不传递文本——所有需要翻译的文本在 Python 层用 `_()` 处理

### Python 部分

**模式**：`_("English")  # cn:中文`，GNU gettext 工具链。

#### DADataAnalysisGui（GUI 层 — 标杆实现）

- `__init__.py` 顶部调用 `setup_i18n()`，在导入业务模块之前
- `i18n/core.py`：语言检测（环境变量 > 系统 locale > Windows API）、翻译加载、全局 `_` 注入
- `i18n/update_po.py`：从代码中 `_("English")  # cn:中文` 注释自动填充 `.po` 翻译
- `update-i18n.sh`：一键生成 `.pot` → 同步 `.po` → 编译 `.mo`
- 业务代码（`dataframe_cleaner.py` 等）所有用户可见文本用 `_()` 包裹

#### DADataAnalysisNodes（节点层）

- `__init__.py` 顶部调用 `setup_i18n()`，在节点模块导入之前
- 每个节点文件的 `@NodeDef(category=...)`、`Parameter(description=...)`、`Input/Output(description=...)` 用 `_()` 包裹
- `@NodeDef(name=...)` 保持英文不翻译（参与 `qualified_name` 序列化）
- `@NodeDef(description=...)` 用 `_()` 包裹翻译，显示在 tooltip 中
- 类 docstring 改为英文（作为 `description` 的降级回退，不经过 `_()` 翻译）
- `paint()` 中硬编码文本用 `_()` 包裹

### i18n 工作流

更新翻译文件的完整流程：

```bash
# 1. 进入对应 Python 包目录
cd PyScripts/DADataAnalysisGui
# 或 cd PyScripts/DADataAnalysisNodes

# 2. 运行一键脚本（需要 Git Bash，含 xgettext/msgmerge/msgfmt）
bash update-i18n.sh

# 3. 用 update_po.py 从 # cn: 注释自动填充空翻译
python i18n/update_po.py i18n/locale/zh_CN/LC_MESSAGES/DADataAnalysisGui.po --py-dir .

# 4. 手动补充剩余空翻译，重新编译
msgfmt -o i18n/locale/zh_CN/LC_MESSAGES/DADataAnalysisGui.mo i18n/locale/zh_CN/LC_MESSAGES/DADataAnalysisGui.po
```

---

## 三、构建与安装

```powershell
# 构建 DataAnalysis 插件
.\scripts\build.ps1 -Target DataAnalysis
```

CMake 安装规则：
- C++ 库 → `bin/<Config>/plugins/`
- Python 包 → `bin/<Config>/pyplugins/`（`install(DIRECTORY PyScripts/...)`）

---

## 四、参考文件

| 文件 | 说明 |
|------|------|
| `DataAnalysisUI.cpp::retranslateUi()` | C++ retranslate 标杆实现 |
| `PyScripts/DADataAnalysisGui/i18n/core.py` | Python i18n 核心实现（标杆） |
| `PyScripts/DADataAnalysisGui/i18n/update_po.py` | `# cn:` 注释自动填充工具 |
| `PyScripts/DADataAnalysisGui/update-i18n.sh` | 一键翻译生成脚本 |
| `PyScripts/DADataAnalysisGui/dataframe_cleaner.py` | `_()` 用法标杆 |
| [docs/zh/dev-guide/python-i18n.md](../../docs/zh/dev-guide/python-i18n.md) | Python i18n 完整规范 |
