# 图标与 UI 设计规范

> **📌 规范正文已统一到仓库根目录 [`DESIGN.md`](https://github.com/czyt1988/data-workbench/blob/dev/DESIGN.md)**
>
> 本页是文档站的入口指引，不再复制正文：同一套视觉规范只在 `DESIGN.md` 一处维护，避免同一件事出现两份说法。

## 阅读方式

- **仓库内浏览**：打开仓库根目录的 `DESIGN.md`（与 `AGENTS.md` 同级）。
- **在线浏览**：[GitHub 上的 DESIGN.md](https://github.com/czyt1988/data-workbench/blob/dev/DESIGN.md)。

## 规范覆盖内容（DESIGN.md 目录）

| 章节 | 内容 |
|------|------|
| §1 | 设计定位与原则 |
| §2 | 色系：官方色板（唯一权威源 `docs/assets/PIC/palette.svg`）、语义色细则、遗留色黑名单 |
| §3 | UI 设计规范：配色映射、控件风格、自绘元素取色先例、内嵌 Web 视图、主题兼容 |
| §4 | 图标设计规范：画布规格、色彩组合规则、命名、SVG 结构要求、图标来源风格 |
| §5 | 现有图标资源清单（先查再画） |
| §6 | AI 工作流与自检 Checklist（含色值扫描等校验命令） |
| §7 | 禁止事项 |
| §8 | 变更流程与维护 |

## 强制规则

**凡涉及界面设计或 SVG 图标设计（新增、替换、修改 `src/DAGui/icon/`、`src/APP/Icon/` 或插件 `icon/` 下的任意 `.svg`），必须先阅读 `DESIGN.md`**，并按其中 §6 的 Checklist 逐项核对。任何新增色值都必须先更新官方色板图并在 `DESIGN.md` 登记（见其 §8 变更流程）。

图标资源归属：通用 SVG 图标放 `src/DAGui/icon/`（界面层 L3），应用级图标放 `src/APP/Icon/`（应用层 L5），详见 [模块依赖关系](../architecture/module-dependency.md)。
