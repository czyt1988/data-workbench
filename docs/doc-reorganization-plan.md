# 文档整理方案与资源审计报告（2026-08）

> 本文档记录 `docs/zh/` 文档重组的执行结果，以及 `docs/assets/` 资源审计结论。
> **第五节的"建议删除"资源清单尚未执行删除，等待确认后再删。**

---

## 一、目录重组结果（已执行）

顶层结构保持不变，`dev-guide/` 内部按主题子目录化，`zh/` 根散文件收拢进子目录：

```
docs/
├── assets/                    # 全部图片/截图/drawio 源文件（原 zh/assets 已合并进来）
│   ├── PIC/                   # 图片
│   ├── screenshot/            # 截图与动图
│   ├── drawio/                # drawio 可编辑源文件
│   └── icon.png / icon.ico    # 站点图标
└── zh/
    ├── index.md               # 首页（原 overview.md 与 index.md 合并，站点首页要求）
    ├── quick-start.md
    ├── structure/             # 项目结构（原根级散文件）
    ├── build/                 # 构建（+ 原根级 large-cmake-project-guide.md）
    ├── dev-guide/
    │   ├── developer-guide.md # 开发指引入口
    │   ├── general/           # 规范：编码/图标/i18n/日志/枚举工具/第三方库管理
    │   ├── architecture/      # 架构与模块：7 篇
    │   ├── workflow/          # 工作流：7 篇
    │   ├── python-binding/    # Python 集成：9 篇（含并入的目录结构说明）
    │   ├── graphics/          # 图形与绘图：4 篇
    │   ├── agent/             # Agent：8 篇
    │   └── ui/                # 界面开发：3 篇
    ├── plugin/                # 插件开发：8 篇（原分散在 zh 根与 dev-guide 的 10 篇合并而来）
    ├── use-guide/             # 使用指南（+ 原根级 configuration.md）
    └── reference/             # 最佳实践/性能/FAQ/贡献指南/术语表/API 索引/更新日志
```

### 内容合并清单（6 组，已执行）

| 被合并文档 | 并入目标 | 说明 |
|---|---|---|
| `zh/index.md` | `zh/index.md`（即原 `overview.md` 改名） | 徽章、核心价值、快速导航并入概览；改名是为满足 mkdocs-static-i18n 对语言首页 `index.md` 的硬性要求 |
| `dev-guide/python-in-cpp.md` | `dev-guide/python-binding/index.md` | 两段 Python 包目录结构并入；导航章节补齐 9 篇系列文档链接 |
| `zh/appendix.md` | 拆解 | CMake 辅助宏 → `build/large-cmake-project-guide.md`；术语表/更新日志/社区/许可证与现有 `glossary.md`、`changelog.md`、`contribution-guide.md` 重复，直接舍弃 |
| `zh/appendix-config.md` | `use-guide/configuration.md` | 仅 UI 状态配置、工作流序列化双轨表两段独有内容并入；`.dawproj` 章节在代码中不存在（虚构格式），弃掉 |
| `dev-guide/plugin-project-create.md` | `plugin/plugin-development.md` | git submodule 细节、更完整的 CMake 示例（含 WIN32/MSVC 处理）、Python-first 插件结构、template.json 字段说明并入 |
| `dev-guide/plugins-interfaces.md` | `plugin/plugin-system.md` | 两张 UML 图与"注意事项"并入；基类详解等重复且过时的内容舍弃 |

### 导航与引用同步（已执行）

- `mkdocs.yml` 导航重写，**补齐 15 篇原孤儿文档**（`logging`、`i18n`、`python-i18n`、`da-enum-string-utils`、`data-module`、`project-file-structure`、`project-serialization-architecture`、`python-multi-threading`、`dapybind11-qt-caster`、`workflow-plugin-discovery`、`node-rendering-settings`、`qwt-serialization-guide`、`agent-runtime-watchdog`、`plugin-architecture` 等，原不在导航中、网站上点不到）。
- 文档间相对链接重写 250 处；外部引用（根 `AGENTS.md` 49 处、`readme.md` 13 处、3 个插件 `AGENTS.md`、`src/DAPyWorkFlow/AGENTS.md`、4 个 skills 文件）共 77 处已同步。
- `mkdocs build` 验证通过：站点页面、首页、资源路径均正常；剩余告警均为既有问题（见第三节）。

---

## 二、资源审计总表

合并后 `docs/assets/` 共 **62 个文件**。状态说明：✅ 被文档/站点引用；🟡 未引用但建议保留；🔴 建议删除。

### 被引用，必须保留（16 个）✅

| 文件 | 引用者 |
|---|---|
| `PIC/about-data-work-flow.png` | readme.md |
| `PIC/app-area.png`、`PIC/uml-interface.png` | dev-guide/architecture/interface-module.md |
| `PIC/build-daworkbench-cmake-qtc-01/02.png` | build/main-program-build.md |
| `PIC/build-daworkbenchplugins-cmake-qtc-01.png` | build/plugin-build.md |
| `PIC/cmake-after-install.png`、`cmake-after-install2.png`、`cmake-standard-install-dir.png` | build/large-cmake-project-guide.md |
| `PIC/palette.svg` | dev-guide/general/icon-ui-design-guide.md |
| `PIC/resible-graphicsitem.png` | dev-guide/graphics/scalable-graphic-module.md |
| `PIC/uml-module-last3.png`、`uml-module-relation.png` | plugin/plugin-system.md |
| `screenshot/01.png`、`02.png` | index.md / quick-start.md / readme.md |
| `screenshot/03.png`、`04.png`、`agent-analysis.gif`、`agent-auto-create-chart.gif` | readme.md |
| `icon.png`、`icon.ico` | mkdocs.yml 主题 logo/favicon |

### 未引用，建议保留（19 个）🟡

| 文件 | 保留理由 |
|---|---|
| `drawio/about-data-work-flow.drawio` | readme 所用 about-data-work-flow.png 的可编辑源 |
| `drawio/about-data-collect-system.drawio`、`data-work-flow-uml.drawio` | UML/架构图可编辑源，重绘时要用 |
| `PIC/build-3rdparty-cmake-qtc-01~04.png`（4 张） | 第三方库构建步骤截图，建议后续挂接到 build/third-party-build.md |
| `PIC/build-error-nopython.png`、`-02/-03/-04.png`、`build-succ-but-unable-run.png`、`error-01.png`（6 张） | 构建排障素材，建议后续挂接到 build/common-build-errors.md 或 python-environment.md |
| `PIC/cmake-qt-dir.png`、`standard-source-dir.png`、`copy-pyscripts.jpg`、`move-py-to-bin-dir.png`（4 张） | 构建/Python 环境配置插图素材 |
| `PIC/screenshot1.png` | 疑似断链 `screenshot1.gif` 的替代图（见第三节），确认后改引用 |
| `screenshot/setting-page-agent-setting.png`、`setting-page-agent-setting-set-baseurl-and-add-model.png`（2 张） | Agent 设置页截图，建议挂接到 use-guide/configuration.md 的 Agent 配置节 |

### 未引用，建议删除（24 个）🔴

| 文件 | 删除理由 |
|---|---|
| `PIC/palette.png` | palette.svg 已引用，PNG 为冗余格式副本 |
| `PIC/app-area-01.png` | 旧版本截图，现行引用的是 app-area.png |
| `PIC/about-data-collect-system.png` | 无任何引用（drawio 源文件保留，可随时重新导出） |
| `PIC/normal-workflow.png` | 无引用 |
| `PIC/workflow.png`、`workflow-scene.png`、`workflow-secene-mouse-press.png`、`workflow-node-beginlink.png`、`workflow-node-cancellink.png`、`workflow-node-create.png`、`workflow-node-finishlink.png`、`workflow-linkpoint-create-update.png`、`workflow-ItemLinkPointSelected.png`、`workflow-regist-nodefactory.png`（10 张） | 旧版工作流内部操作截图，相关工作流文档均未使用，UI 已演进，如需要应重新截图 |
| `PIC/uml-Plugin.png`、`uml-workflow.png`、`uml-workflow-graphicsview.png`、`uml-工作流相关模块关系.png`（4 张） | 未被使用的旧 UML 导出图（被引用的是 uml-interface / uml-module-* 系列；drawio 源文件保留） |
| `PIC/插件加载过程生命周期.png`、`流程仿真框架.png`（2 张） | 中文文件名旧图，无引用，且不符合命名规范 |
| `PIC/workflow-ui/comfyui.png`、`comfyui-02.png`、`orange3.png`、`orange3-2.jpg`（4 张） | 竞品（ComfyUI/Orange3）参考截图，无任何文档引用；如需保留设计参考建议移出版本库另行存放 |

另：`drawio/~$about-data-work-flow.drawio.bkp`（Office 类临时备份文件）已在迁移时删除。

---

## 三、断链清单（引用了不存在的文件，未改动，待决策）

| 引用位置 | 目标 | 建议 |
|---|---|---|
| `zh/index.md`（动态演示图） | `assets/screenshot/screenshot1.gif` | 仓库现存 `PIC/screenshot1.png`，疑似其替代图——确认后把引用改为该 PNG（并决定是否移入 screenshot/） |
| `zh/build/plugin-build.md` | `assets/PIC/plugin-list.png` | 图片缺失，需补一张插件列表现截图或删掉该引用 |
| `docs/doc-build.md` | `assets/screenshot/workflow-demo.png` | 图片缺失（出现 3 次），需补图或删引用 |

既有告警（与本次整理无关，仅记录）：若干文档链接到 `src/**` 源码文件（GitHub 上可点，站点上天然无法渲染）；`doxygen/*.html` 占位链接在本地构建时不存在（CI 部署时生成）。

---

## 四、后续内容合并候选（本次未动，仅标注）

| 候选 | 说明 |
|---|---|
| `plugin/plugin-architecture.md`（1982 行） vs `plugin/plugin-system.md` | 两者都讲插件框架，前者是长篇深度解析，疑有重叠；篇幅大、合并风险高，建议单独开任务处理 |
| `dev-guide/architecture/data-module.md`、`interface-module.md` vs `module-breakdown.md` 对应章节 | 模块专题文档与模块总览存在局部重叠，可后续比对去重 |

---

## 五、待确认事项

1. **第二节 🔴 的 24 个文件是否全部删除**（或指出需要保留的例外）。
2. 第三节 3 处断链的处理方式（补图 / 改引用 / 删引用）。
3. `screenshot1.png` 是否就是 `screenshot1.gif` 的替代图。
