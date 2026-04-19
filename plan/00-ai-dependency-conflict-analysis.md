# AI 依赖冲突预检分析文档

> **文档性质**：基于已知包兼容性数据的理论分析，非实际 pip install 测试
> **分析日期**：2026-04-19
> **分析目标**：评估 crewAI + LangChain/LangGraph 与现有 requirements.txt 依赖的兼容性，预判冲突点并提出解决方案

---

## 1. 当前 requirements.txt 依赖清单分析

### 1.1 现有依赖列表

```
numpy
loguru
pandas
scipy
openpyxl
chardet
PyWavelets
pyarrow
typing_extensions
matplotlib
seaborn
langgraph
langchain-openai
langgraph-cli[inmem]
```

### 1.2 关键特征

| 特征 | 说明 |
|------|------|
| **无版本锁定** | 所有包均未指定具体版本号，使用 pip 默认最新版 |
| **Python 版本暗示** | `typing_extensions` 的存在暗示当前目标 Python 3.7+（用于 `Literal` 类型兼容） |
| **AI 依赖已部分引入** | 已包含 `langgraph`、`langchain-openai`、`langgraph-cli[inmem]`，说明 LangChain/LangGraph 已进入项目 |
| **数据科学栈** | numpy + pandas + scipy + matplotlib + seaborn 构成完整数据分析栈 |
| **无 crewAI** | crewAI 尚未引入，这是本次新增的目标包 |

### 1.3 现有依赖的潜在版本范围（pip 默认安装最新版）

| 包名 | 当前最新稳定版 | 说明 |
|------|---------------|------|
| numpy | 2.x | Python 3.10+ 支持 numpy 2.x，但 pandas 可能限制 numpy 版本 |
| pandas | 2.2.x | 需要 numpy>=1.22.4 |
| scipy | 1.14.x | 需要 numpy>=1.22.4 |
| openpyxl | 3.1.x | 无特殊依赖限制 |
| pyarrow | 18.x | 需要 numpy |
| matplotlib | 3.10.x | 需要 numpy |
| langgraph | 1.0.x/1.1.x | 需要 Python>=3.10, langchain-core>=1.2.x |
| langchain-openai | 1.1.x | 需要 openai>=2.26.0, langchain-core>=1.2.21 |

---

## 2. crewAI 依赖树分析

### 2.1 基本版本要求

| 约束 | 值 | 影响级别 |
|------|-----|---------|
| **Python 版本** | `>=3.10, <3.14` | 🔴 **硬性阻断** — 当前项目暗示 Python 3.7+ 目标 |
| **pydantic** | `~=2.11.9`（即 >=2.11.9, <2.12.0） | 🔴 **极窄锁定** — 与 LangChain 的 pydantic 需求冲突 |
| **openai** | `>=1.83.0, <3` | 🟡 **近期放宽** — 从之前的 `<1.84.dev0` 放宽到 `<3`，但历史版本仍有窄锁定 |
| **instructor** | `>=1.3.3` | 🟢 低冲突风险 |
| **chromadb** | `~=1.1.0` | 🟡 可能与 numpy 版本交互 |
| **openpyxl** | `~=3.1.5` | 🟢 与现有 openpyxl 兼容 |
| **uv** | `~=0.9.13` | 🟡 引入 uv 作为依赖，较重 |
| **lancedb** | `>=0.29.2` / `>=0.4.0` | 🟡 数据存储依赖 |
| **pydantic-settings** | `~=2.10.1` | 🟡 需与 pydantic 版本对齐 |

### 2.2 crewAI 核心依赖树（深度展开）

```
crewAI (最新版，基于 pyproject.toml e21c5062)
├── pydantic ~=2.11.9          ← 🔴 极窄锁定
│   ├── pydantic-core ~=2.11.9
│   └── annotated-types
├── openai >=1.83.0,<3         ← 🟡 已放宽但仍需注意
│   ├── httpx
│   ├── anyio
│   └── distro
├── instructor >=1.3.3         ← pydantic 交互
├── chromadb ~=1.1.0           ← 可能拉入 numpy 依赖
│   ├── onnxruntime
│   └── tokenizers
├── pydantic-settings ~=2.10.1 ← 需与 pydantic 版本对齐
├── opentelemetry-api/sdk ~=1.34.0
├── textual >=7.5.0            ← TUI 框架
├── mcp ~=1.26.0               ← MCP 协议支持
├── uv ~=0.9.13                ← 包管理器作为依赖
├── lancedb >=0.29.2           ← 数据存储
├── openpyxl ~=3.1.5           ← 与现有依赖兼容 ✅
└── pdfplumber ~=0.11.4
```

### 2.3 crewAI 与现有依赖的冲突点

| 冲突点 | 严重性 | 详细说明 |
|--------|--------|---------|
| **Python >=3.10** | 🔴 阻断 | crewAI 硬性要求 Python >=3.10，当前项目暗示 3.7+ 目标 |
| **pydantic ~=2.11.9** | 🔴 高危 | 极窄版本锁定（2.11.9 ~ <2.12.0），与 LangChain 的 pydantic>=2.5.2,<3.0.0 范围交集极小 |
| **openai 版本** | 🟡 中等 | crewAI 当前放宽到 openai>=1.83.0,<3，与 langchain-openai 的 openai>=2.26.0,<3.0.0 有交集 |
| **numpy 兼容性** | 🟡 中等 | crewAI 的 chromadb 依赖可能引入 numpy 版本要求，与 pandas 的 numpy 需求可能冲突 |
| **chromadb** | 🟡 中等 | chromadb~=1.1.0 引入 onnxruntime 等较重的依赖 |
| **typing_extensions** | 🟢 低 | Python 3.10+ 后 typing_extensions 可移除 |

---

## 3. LangChain/LangGraph 依赖树分析

### 3.1 LangGraph 核心依赖

| 约束 | 值 | 影响级别 |
|------|-----|---------|
| **Python 版本** | `>=3.10` | 🔴 硬性要求 |
| **langchain-core** | `>=1.2.21, <2.0.0` | 🔴 关键约束 — 决定整个 LangChain 生态版本 |
| **pydantic** | `>=2.5.2, <3.0.0` | 🟡 范围较宽，但需与 crewAI 对齐 |
| **langsmith** | `>=0.1.117, <2.0.0` | 🟢 低冲突 |

### 3.2 langchain-openai 核心依赖

| 约束 | 值 | 影响级别 |
|------|-----|---------|
| **Python 版本** | `>=3.10.0, <4.0.0` | 🔴 硬性要求 |
| **langchain-core** | `>=1.2.21, <2.0.0` | 🔴 与 LangGraph 对齐 |
| **openai** | `>=2.26.0, <3.0.0` | 🔴 **关键冲突点** |
| **tiktoken** | `>=0.7.0, <1.0.0` | 🟢 低冲突 |

### 3.3 LangChain 生态依赖树

```
langgraph >=1.0.x
├── langchain-core >=1.2.21,<2.0.0    ← 🔴 核心锚点
│   ├── pydantic >=2.5.2,<3.0.0       ← 范围较宽
│   ├── langsmith >=0.1.117
│   ├── tenacity >=8.1.0,<9.0.0
│   └── jsonpatch >=1.33,<2.0.0
├── langgraph-checkpoint >=4.0.0
└── langgraph-prebuilt >=1.0.x
    └── langchain-core >=1.0.0

langchain-openai >=1.1.x
├── langchain-core >=1.2.21,<2.0.0    ← 与 langgraph 对齐 ✅
├── openai >=2.26.0,<3.0.0            ← 🔴 与 crewAI 的 openai 冲突
├── tiktoken >=0.7.0,<1.0.0
└── httpx                              ← openai 的依赖
```

### 3.4 LangChain/LangGraph 与 crewAI 的冲突点

| 冲突点 | 严重性 | 详细说明 |
|--------|--------|---------|
| **openai 版本** | 🔴 **核心冲突** | langchain-openai 要求 `openai>=2.26.0,<3.0.0`；crewAI 早期版本锁定 `openai>=1.83.0,<1.84.dev0`（v1.9.2），最新版放宽为 `openai>=1.83.0,<3`。交集存在但需 crewAI 最新版 |
| **pydantic 版本** | 🔴 **核心冲突** | crewAI 锁定 `pydantic~=2.11.9`（极窄：2.11.9 ~ <2.12.0）；LangChain 需要 `pydantic>=2.5.2,<3.0.0`。交集：pydantic 2.11.9 满足 LangChain 范围 ✅，但锁定极窄意味着 pip 必须安装恰好 2.11.9 |
| **langchain-core 版本** | 🟡 中等 | crewAI 最新版不再直接依赖 langchain-core（已移除），但 LangGraph/LangChain-openai 强制 langchain-core>=1.2.21 |
| **Python 3.10 硬性要求** | 🔴 阻断 | LangGraph 和 crewAI 均要求 Python>=3.10 |

### 3.5 LangChain/LangGraph 与现有依赖的冲突点

| 冲突点 | 严重性 | 详细说明 |
|--------|--------|---------|
| **numpy 版本** | 🟡 中等 | langchain-core 无直接 numpy 依赖，但 pandas 的 numpy 需求可能与 chromadb 等间接依赖冲突 |
| **typing_extensions** | 🟢 低 | Python 3.10+ 后不再需要 typing_extensions 补充 Literal |
| **matplotlib/seaborn** | 🟢 低 | 与 LangChain 无依赖交互 |

---

## 4. 已知的依赖冲突历史

### 4.1 openai 版本锁定冲突（最严重）

**历史轨迹**：

| crewAI 版本 | openai 约束 | 与 langchain-openai 兼容性 |
|-------------|-------------|--------------------------|
| 0.1.x (2024-01) | `openai>=1.7.1,<2.0.0` | ❌ 与 langchain-openai>=0.0.5 冲突 |
| 0.11.x (2024-02) | 间接通过 langchain-openai<0.0.6 | ❌ 锁定极窄 |
| 0.51.x (2024-08) | 仍使用较窄范围 | ❌ 与 langchain-ollama 冲突 |
| 1.9.2 (2026-01) | `openai>=1.83.0,<1.84.dev0` | ❌ 与 langchain-openai>=1.0.3 冲突（需 openai>=1.109.1） |
| **最新版 (e21c5062)** | `openai>=1.83.0,<3` | ⚠️ 部分兼容 — 与 langchain-openai 的 `openai>=2.26.0,<3.0.0` 交集为 [2.26.0, 3.0) |

**关键 Issue**：
- [#4300](https://github.com/crewAIInc/crewAI/issues/4300)：过度严格的依赖约束导致与 langchain-openai>=1.0.3 无法共存
- [#259](https://github.com/crewAIInc/crewAI/issues/259)：crewAI 与最新 langchain-openai 冲突
- [#1911](https://github.com/crewAIInc/crewAI/issues/1911)：crewAI 与 langchain-ollama 依赖冲突

### 4.2 pydantic 版本冲突

**历史轨迹**：

| crewAI 版本 | pydantic 约束 | 与 LangChain 兼容性 |
|-------------|---------------|-------------------|
| 0.x 系列 | `pydantic>=2.4.2,<3.0.0` | ⚠️ 范围宽但低于 2.8 有 bug |
| [#3011 bug](https://github.com/crewAIInc/crewAI/issues/3011) | `pydantic>=2.4.2` 但 <2.8 有兼容性问题 | ❌ 2.4.2-2.7.x 不稳定 |
| **最新版** | `pydantic~=2.11.9` | ⚠️ 极窄锁定（仅 2.11.9），与 LangChain 的 `>=2.5.2,<3.0.0` 交集为 {2.11.9} |

**核心问题**：crewAI 使用 `~=` 操作符锁定 pydantic 到极窄范围（2.11.9 ≤ x < 2.12.0），这意味着 pydantic 版本必须恰好是 2.11.9 系列。此版本在 LangChain 的 `>=2.5.2,<3.0.0` 范围内，技术上兼容，但极窄锁定导致：
- pip resolver 可能需要多次回溯尝试
- 如果 pydantic 2.11.9 不存在或有问题，整个安装链断裂
- 未来 crewAI 更新可能改变锁定版本

### 4.3 langchain-core 版本级联冲突

**历史问题**：
- crewAI 早期版本直接依赖 langchain（如 langchain<0.2.0,>=0.1.0），与 langchain-openai 的 langchain-core 级联冲突
- [#1442](https://github.com/crewAIInc/crewAI/issues/1442)：crewai 与 crewai-tools 的 langchain 版本不一致
- 最新版 crewAI **已移除** langchain 直接依赖，但 LangChain 生态内部的 langchain-core 版本升级可能引入新的约束

**当前状态**：
- langchain-core 最新版为 1.2.x / 1.3.x 系列
- langchain-openai 1.1.14 要求 langchain-core>=1.2.21,<2.0.0
- LangGraph 1.0.x/1.1.x 要求 langchain-core>=1.2.x
- crewAI 不再依赖 langchain-core（已移除），减少了冲突面 ✅

### 4.4 CVE 安全修复导致版本提升

- **CVE-2026-26013**：langchain-core <1.2.11 存在 SSRF 漏洞，需 >=1.2.11
- **CVE-2026-4539**：pygments<2.20.0 存在安全漏洞
- crewAI 的 uv override 中已包含 `langchain-core>=1.2.11,<2` 的安全约束

---

## 5. 推荐的兼容版本锁定方案

### 5.1 核心原则

1. **先锁定 AI 生态版本**，再调整数据科学栈版本
2. **优先满足 crewAI 的窄约束**（因为更难兼容），再确保 LangChain 在范围内
3. **所有版本必须指定确切版本号或窄范围**，避免 pip resolver 回溯失败
4. **使用 Python 3.10+** 作为硬性前提（已在 Task 0.1 中确认）

### 5.2 推荐版本锁定方案

#### AI 生态包（核心冲突区域）

| 包名 | 推荐版本/范围 | 约束来源 | 说明 |
|------|---------------|---------|------|
| **crewai** | `>=1.14.0` | crewAI 最新稳定版 | 必须使用最新版以获得放宽的 openai 约束 |
| **pydantic** | `2.11.9` | crewAI 硬性锁定 | 必须锁定此版本以满足 crewAI ~=2.11.9 |
| **pydantic-core** | `2.11.9`（自动匹配） | pydantic 依赖 | 与 pydantic 版本自动对齐 |
| **openai** | `>=2.26.0,<3.0.0` | langchain-openai 约束 | 需满足 langchain-openai 的最低要求，同时在 crewAI 的 >=1.83.0,<3 范围内 |
| **langchain-core** | `>=1.2.21,<2.0.0` | langchain-openai + LangGraph | 安全约束需 >=1.2.11（CVE-2026-26013） |
| **langchain-openai** | `1.1.14` | 项目已引入 | 最新稳定版 |
| **langgraph** | `1.0.10` 或 `1.1.7a1` | 项目已引入 | 1.0.10 为最新稳定版，1.1.x 为 alpha |
| **langgraph-cli** | `[inmem]` 最新版 | 项目已引入 | 无版本冲突 |
| **tiktoken** | `>=0.7.0,<1.0.0` | langchain-openai | token 计数库 |
| **instructor** | `>=1.3.3` | crewAI 依赖 | pydantic 交互库 |

#### 数据科学包（需与 AI 生态对齐）

| 包名 | 推荐版本/范围 | 说明 |
|------|---------------|------|
| **numpy** | `>=1.24.0,<2.0.0` | pandas 2.2.x + scipy 兼容；crewAI 的 chromadb 可能需要 numpy<2 |
| **pandas** | `2.2.3` | crewAI pandas optional dependency 也锁定 2.2.3 ✅ |
| **scipy** | `>=1.10.0,<1.15.0` | 与 numpy>=1.24 兼容 |
| **matplotlib** | `>=3.7.0,<3.10.0` | Python 3.10 兼容 |
| **seaborn** | `>=0.12.0,<0.14.0` | 与 matplotlib 兼容 |
| **openpyxl** | `3.1.5` | crewAI 也锁定此版本 ✅ 完全一致 |
| **pyarrow** | `>=14.0.0,<19.0.0` | 与 pandas 2.2.x 兼容 |
| **PyWavelets** | `>=1.5.0,<2.0.0` | Python 3.10+ 兼容 |
| **chardet** | `>=5.0.0,<6.0.0` | 无特殊约束 |
| **loguru** | `>=0.7.0,<1.0.0` | 无特殊约束 |
| **typing_extensions** | **移除** | Python 3.10+ 不再需要，直接使用 `from typing import Literal` |

#### crewAI 附加依赖（自动拉入）

| 包名 | 版本范围 | 说明 |
|------|---------|------|
| chromadb | ~=1.1.0 | crewAI 自动拉入 |
| tokenizers | >=0.21,<1 | crewAI 自动拉入 |
| opentelemetry-api/sdk | ~=1.34.0 | crewAI 自动拉入 |
| textual | >=7.5.0 | crewAI TUI |
| uv | ~=0.9.13 | crewAI 包管理器依赖 |
| lancedb | >=0.29.2 | crewAI 数据存储 |

### 5.3 冲突解决的决策矩阵

| 冲突 | 解决策略 | 优先级 |
|------|---------|--------|
| **openai 版本** | 使用 crewAI 最新版（openai>=1.83.0,<3），安装 openai>=2.26.0 满足 langchain-openai | 🔴 最高 |
| **pydantic 版本** | 锁定 pydantic==2.11.9，满足 crewAI ~=2.11.9 和 LangChain >=2.5.2,<3.0.0 | 🔴 最高 |
| **Python 3.10** | 升级 Python 运行环境到 3.10+（已在 Task 0.1 中确认） | 🔴 最高 |
| **numpy 版本** | 使用 numpy>=1.24,<2.0 以兼容 pandas+scipy+chromadb | 🟡 中等 |
| **typing_extensions** | Python 3.10+ 后移除，简化 requirements.txt | 🟢 低 |

---

## 6. 安装顺序建议

### 6.1 推荐安装顺序（避免 pip resolver 回溯失败）

**核心原则**：先安装约束最窄的包，再安装约束较宽的包，让 pip resolver 从窄到宽匹配。

```
步骤 1：创建干净的 Python 3.10+ 环境
─────────────────────────────────────────
python3.10 -m venv .venv
source .venv/bin/activate  # Linux/Mac
# 或 .venv\Scripts\activate  # Windows

步骤 2：安装 pydantic（最窄约束的锚点）
─────────────────────────────────────────
pip install pydantic==2.11.9
# 此版本满足：
#   - crewAI ~=2.11.9 (即 >=2.11.9, <2.12.0) ✅
#   - LangChain >=2.5.2, <3.0.0 ✅

步骤 3：安装 crewAI（窄约束框架）
─────────────────────────────────────────
pip install crewai>=1.14.0
# 注意：不使用 crewai[tools]，工具包可能引入额外冲突
# crewAI 会自动拉入 pydantic~=2.11.9（已预装 ✅）、openai>=1.83.0,<3 等

步骤 4：安装 LangChain/OpenAI 生态
─────────────────────────────────────────
pip install langchain-core>=1.2.21,<2.0.0
pip install langchain-openai==1.1.14
pip install langgraph==1.0.10
pip install "langgraph-cli[inmem]"
# openai>=2.26.0,<3.0.0 将被 langchain-openai 自动拉入
# 与 crewAI 的 openai>=1.83.0,<3 交集为 [2.26.0, 3.0) ✅

步骤 5：安装数据科学栈
─────────────────────────────────────────
pip install "numpy>=1.24.0,<2.0.0"
pip install pandas==2.2.3
pip install "scipy>=1.10.0,<1.15.0"
pip install "matplotlib>=3.7.0,<3.10.0"
pip install "seaborn>=0.12.0,<0.14.0"
pip install openpyxl==3.1.5
pip install "pyarrow>=14.0.0,<19.0.0"
pip install "PyWavelets>=1.5.0,<2.0.0"
pip install "chardet>=5.0.0,<6.0.0"
pip install "loguru>=0.7.0,<1.0.0"

步骤 6：验证安装完整性
─────────────────────────────────────────
pip check
python -c "import crewai; print(f'crewAI {crewai.__version__}')"
python -c "import langchain_openai; print(f'langchain-openai OK')"
python -c "import langgraph; print(f'langgraph OK')"
python -c "import pandas; print(f'pandas {pandas.__version__}')"
python -c "import numpy; print(f'numpy {numpy.__version__}')"
```

### 6.2 为什么此顺序有效

| 步骤 | 原因 |
|------|------|
| pydantic 先装 | crewAI 的 ~=2.11.9 是最窄约束，必须先锚定 |
| crewAI 第二 | 其 openai>=1.83.0,<3 范围包含 langchain-openai 需要的 >=2.26.0 |
| LangChain 第三 | langchain-core>=1.2.21 是 LangChain 生态的版本锚点 |
| 数据科学栈最后 | 这些包约束较宽，兼容性最好 |

### 6.3 不推荐的安装顺序

❌ **一次性 pip install -r requirements.txt**：pip resolver 面对窄约束时回溯成本高，容易选择不兼容版本

❌ **先装 LangChain 再装 crewAI**：LangChain 的 pydantic>=2.5.2,<3.0.0 范围太宽，pip 可能选择 2.8.x 版本，与 crewAI ~=2.11.9 冲突

❌ **先装数据科学栈**：numpy 2.x 可能被 pip 选择，导致 chromadb 等不兼容

---

## 7. pip 安装验证命令清单

### 7.1 前置检查

```bash
# 检查 Python 版本（必须 >=3.10, <3.14）
python --version
# 期望输出: Python 3.10.x / 3.11.x / 3.12.x / 3.13.x

# 检查 pip 版本（建议 >=24.0 以获得更好的 resolver）
pip --version
# 如果版本过旧：
pip install --upgrade pip
```

### 7.2 逐步安装验证

```bash
# 步骤 A: 验证 pydantic 安装
pip install pydantic==2.11.9
python -c "from pydantic import BaseModel; print(f'pydantic {__import__(\"pydantic\").__version__}')"
# 期望输出: pydantic 2.11.9

# 步骤 B: 验证 crewAI 安装
pip install "crewai>=1.14.0"
python -c "import crewai; print(f'crewAI {crewai.__version__}')"
# 期望输出: crewAI 版本号 >=1.14.0

# 步骤 C: 验证 openai 版本满足交集
pip show openai
# 期望: Version: >=2.26.0（满足 langchain-openai 和 crewAI 的共同范围）

# 步骤 D: 验证 LangChain 生态
pip install "langchain-core>=1.2.21,<2.0.0"
pip install langchain-openai==1.1.14
pip install langgraph==1.0.10
pip install "langgraph-cli[inmem]"

python -c "import langchain_core; print(f'langchain-core {langchain_core.__version__}')"
python -c "import langchain_openai; print(f'langchain-openai OK')"
python -c "import langgraph; print(f'langgraph {langgraph.__version__}')"
# 期望: langchain-core >=1.2.21, 所有模块正常导入

# 步骤 E: 验证数据科学栈
pip install "numpy>=1.24.0,<2.0.0" pandas==2.2.3 scipy matplotlib seaborn openpyxl pyarrow PyWavelets chardet loguru

python -c "import numpy; print(f'numpy {numpy.__version__}')"
python -c "import pandas; print(f'pandas {pandas.__version__}')"
python -c "import scipy; print(f'scipy OK')"
python -c "import matplotlib; print(f'matplotlib OK')"
# 期望: numpy <2.0.0, pandas 2.2.3
```

### 7.3 交叉验证命令

```bash
# 验证依赖一致性（最关键的检查）
pip check
# 期望输出: 无冲突（No broken requirements found）

# 验证 crewAI + LangChain 共存
python -c "
from crewai import Agent, Task, Crew, Process
from langchain_openai import ChatOpenAI
from langgraph.graph import StateGraph, START, END
print('crewAI + LangChain + LangGraph 共存验证 ✅')
"
# 期望: 无 ImportError

# 验证 pydantic 版本一致性
python -c "
import pydantic
assert pydantic.__version__.startswith('2.11'), f'pydantic 版本应为 2.11.x，实际为 {pydantic.__version__}'
print(f'pydantic {pydantic.__version__} ✅')
"

# 验证 openai 版本在交集范围
python -c "
import openai
v = openai.__version__
assert v >= '2.26.0', f'openai 版本应 >=2.26.0，实际为 {v}'
print(f'openai {v} ✅（满足 crewAI >=1.83.0,<3 和 langchain-openai >=2.26.0,<3.0.0）')
"

# 验证 pandas 与 crewAI 兼容
python -c "
import pandas
assert pandas.__version__ == '2.2.3', f'pandas 版本应为 2.2.3，实际为 {pandas.__version__}'
print(f'pandas {pandas.__version__} ✅（与 crewAI pandas optional dep 一致）')
"

# 验证 numpy 版本
python -c "
import numpy
v = float(numpy.__version__.split('.')[0] + '.' + numpy.__version__.split('.')[1])
assert v >= 1.24 and v < 2.0, f'numpy 版本应在 [1.24, 2.0)，实际为 {numpy.__version__}'
print(f'numpy {numpy.__version__} ✅')
"
```

### 7.4 冻结完整依赖清单

```bash
# 安装完成后，冻结依赖以便复现
pip freeze > requirements-lock.txt
# 此文件可用于未来环境复现，但不应替代 requirements.txt
```

### 7.5 回滚命令（如果安装失败）

```bash
# 如果遇到无法解决的冲突，重建环境
pip freeze > broken-requirements.txt  # 保存当前状态供排查
pip uninstall -y crewai langchain-openai langgraph langchain-core  # 移除冲突包
# 或直接重建虚拟环境：
deactivate
rm -rf .venv  # Linux/Mac
# 或 Remove-Item -Recurse .venv  # Windows PowerShell
python3.10 -m venv .venv
# 然后按 6.1 的顺序重新安装
```

---

## 8. 风险评估与缓解策略

### 8.1 高风险项

| 风险 | 影响 | 缓解策略 |
|------|------|---------|
| **crewAI pydantic 极窄锁定** | 如果 pydantic 2.11.9 不可用或有 bug，整个安装链断裂 | 关注 crewAI 的 pyproject.toml 更新；必要时 fork 并放宽约束 |
| **crewAI 版本更新频繁** | 新版本可能改变约束范围 | 锁定 crewAI 具体版本号而非范围 |
| **openai SDK 大版本升级** | openai 3.x 可能引入 API 变化 | 当前 <3 约束已保护；长期需关注 openai 3.x 兼容性 |

### 8.2 中等风险项

| 风险 | 影响 | 缓解策略 |
|------|------|---------|
| **chromadb 引入重依赖** | onnxruntime 等增大包体积 | 仅在需要 crewAI 内存功能时安装 |
| **numpy 2.x 兼容性** | chromadb、scipy 可能不兼容 numpy 2.x | 锁定 numpy<2.0.0 |
| **Windows 兼容性** | onnxruntime、chromadb-hnswlib 在 Windows 编译问题 | 预装编译工具链或使用预编译 wheel |

### 8.3 低风险项

| 风险 | 影响 | 缓解策略 |
|------|------|---------|
| **typing_extensions 移除** | 现有代码中的 `from typing_extensions import Literal` 需改为 `from typing import Literal` | 已在 Task 0.1 中确认需要修改 dataframe.py |
| **langchain-core 安全漏洞** | CVE-2026-26013 需 >=1.2.11 | 锁定 >=1.2.21 已覆盖 |

---

## 9. 总结与行动建议

### 9.1 核心结论

1. **crewAI 与 LangChain/LangGraph 可以共存**，但必须使用 crewAI 最新版（>=1.14.0），且 pydantic 必须锁定到 2.11.9
2. **openai 版本冲突已部分解决**：crewAI 最新版放宽到 `openai>=1.83.0,<3`，与 langchain-openai 的 `openai>=2.26.0,<3.0.0` 有交集 [2.26.0, 3.0)
3. **Python 3.10 升级是硬性前提**，已在 Task 0.1 中确认
4. **pydantic 极窄锁定是最大隐患**：crewAI 的 `~=2.11.9` 使得版本选择余地极小
5. **numpy 必须锁定 <2.0.0** 以兼容 chromadb/scipy

### 9.2 建议的 requirements.txt 更新方向（仅供参考，不实际修改）

```txt
# AI 生态（带版本锁定）
crewai>=1.14.0
pydantic==2.11.9
openai>=2.26.0,<3.0.0
langchain-core>=1.2.21,<2.0.0
langchain-openai==1.1.14
langgraph==1.0.10
langgraph-cli[inmem]

# 数据科学栈（带版本锁定）
numpy>=1.24.0,<2.0.0
pandas==2.2.3
scipy>=1.10.0,<1.15.0
matplotlib>=3.7.0,<3.10.0
seaborn>=0.12.0,<0.14.0
openpyxl==3.1.5
pyarrow>=14.0.0,<19.0.0
PyWavelets>=1.5.0,<2.0.0
chardet>=5.0.0,<6.0.0
loguru>=0.7.0,<1.0.0

# 注意：typing_extensions 已移除（Python 3.10+ 不再需要）
```

### 9.3 下一步行动

1. 完成 Python 3.10 环境升级（Task 0.1 的执行阶段）
2. 在新环境中按本文档的安装顺序执行验证
3. 根据验证结果微调版本锁定方案
4. 更新 requirements.txt 为带版本锁定的格式
5. 将 requirements-lock.txt 加入版本管理

---

> **文档状态**：预检分析完成，待实际环境验证
> **后续更新**：在实际 pip install 验证后，根据结果更新版本建议