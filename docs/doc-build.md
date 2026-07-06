# 文档构建指南

- ✅ **本地预览**：一键启动文档站点本地预览
- ✅ **构建部署**：生成静态站点并部署到 GitHub Pages
- ✅ **框架配置**：完整的 MkDocs Material 配置说明
- ✅ **内网支持**：mermaid 图表和 KaTeX 公式使用本地静态资源，无需外网访问

本文档说明如何本地预览、构建和部署 data-workbench 项目文档站点。

---

## 前置依赖

项目文档使用 [MkDocs Material](https://squidfunk.github.io/mkdocs-material/) 框架构建。

```bash
# 安装 MkDocs、Material 主题和 mermaid2 插件
pip install mkdocs-material mkdocs-mermaid2-plugin

# 可选：安装 i18n 插件（项目 mkdocs.yml 已配置）
pip install mkdocs-static-i18n

# 验证安装
mkdocs --version
```

!!! note "内网环境说明"
    本文档站点使用本地静态资源渲染 mermaid 图表和 KaTeX 数学公式，**无需外网 CDN 访问**。
    相关 JS/CSS 文件已放置在 `docs/js/` 和 `docs/css/` 目录中，请勿删除。

---

## 本地预览

在项目根目录执行以下命令：

```bash
mkdocs serve
```

启动后访问 `http://localhost:8000` 即可预览文档站点。修改文档内容会自动热刷新。

!!! tip "指定端口"
    如果 8000 端口被占用，可以指定其他端口：
    ```bash
    mkdocs serve --dev-addr localhost:8080
    ```

---

## 构建静态站点

```bash
# 构建到 site/ 目录
mkdocs build

# 构建并清理旧文件
mkdocs build --clean
```

构建产物在 `site/` 目录下，可直接部署到任意静态文件服务器。

---

## 部署

### GitHub Pages（推荐）

项目已配置 GitHub Actions（`.github/workflows/page.yml`），推送到指定分支后自动部署。

也可以手动一键部署：

```bash
# 一键部署到 GitHub Pages
mkdocs gh-deploy
```

此命令会构建站点并推送到 `gh-pages` 分支。

### 手动部署

```bash
# 构建后将 site/ 目录内容上传到服务器
mkdocs build --clean
# 将 site/ 目录内容复制到 Web 服务器根目录
```

---

## 配置文件说明

文档配置在项目根目录的 `mkdocs.yml` 文件中。主要配置项：

| 配置项 | 说明 |
|--------|------|
| `site_name` | 站点名称（DAWorkBench） |
| `theme` | Material 主题配置（颜色方案、语言、功能开关） |
| `nav` | 导航栏结构（文档目录树） |
| `extra_javascript` | 本地 JS 资源（mermaid、KaTeX 离线渲染引擎） |
| `extra_css` | 本地 CSS 资源（公式样式、文档样式增强） |
| `markdown_extensions` | Markdown 扩展（admonition、代码高亮、mermaid、arithmatex 等） |
| `plugins` | 插件配置（搜索、i18n 国际化、mermaid2 离线渲染） |

### 内网静态资源

`mkdocs.yml` 通过 `extra_javascript` 和 `extra_css` 引用本地静态文件（非 CDN），确保内网环境下图表和公式正常渲染。这些文件位于 `docs/js/` 和 `docs/css/` 目录：

| 文件 | 位置 | 用途 |
|------|------|------|
| `mermaid.min.js` | `docs/js/` | mermaid 图表渲染引擎（离线版） |
| `katex.min.js` | `docs/js/` | KaTeX 数学公式渲染引擎（离线版） |
| `auto-render.min.js` | `docs/js/` | KaTeX 自动渲染辅助脚本 |
| `katex-init.js` | `docs/js/` | KaTeX 渲染初始化配置 |
| `extra.css` | `docs/css/` | 文档样式增强（中文字体、表格、代码块等） |
| `katex.min.css` | `docs/css/` | KaTeX 公式样式 |

`mkdocs.yml` 中相关配置：

```yaml
extra_javascript:
  - js/mermaid.min.js
  - js/katex.min.js
  - js/auto-render.min.js
  - js/katex-init.js
extra_css:
  - stylesheets/extra.css
  - css/extra.css
  - css/katex.min.css

plugins:
  - search
  - i18n:
      languages:
        - locale: zh
          name: 中文
          default: true
  - mermaid2

markdown_extensions:
  - pymdownx.superfences:
      custom_fences:
        - name: mermaid
          class: mermaid
          format: !!python/name:mermaid2.fence_mermaid_custom
  - pymdownx.arithmatex:
      generic: true
      smart_dollar: true
```

---

## 文档目录结构

```
data-workbench/
├── mkdocs.yml               # MkDocs 配置文件
├── docs/
│   ├── doc-build.md          # 本文件
│   ├── index.html            # 根目录重定向
│   ├── stylesheets/          # 主题样式
│   │   └── extra.css         # Material 主题额外样式
│   ├── js/                   # 离线 JS 资源
│   │   ├── mermaid.min.js    # mermaid 图表引擎
│   │   ├── katex.min.js      # KaTeX 公式引擎
│   │   ├── auto-render.min.js
│   │   └── katex-init.js
│   ├── css/                  # 离线 CSS 资源
│   │   ├── extra.css         # 文档样式增强
│   │   └── katex.min.css     # KaTeX 公式样式
│   ├── assets/               # 截图和图标资源
│   └── zh/                   # 中文文档
│       ├── index.md          # 文档首页
│       ├── overview.md       # 项目概览
│       ├── quick-start.md    # 快速上手
│       ├── build/            # 构建指南
│       ├── dev-guide/        # 开发指南
│       ├── use-guide/        # 使用指南
│       └── ...
```

---

## 常见问题

### 文档站点启动失败

**症状**：`mkdocs serve` 报错退出

**排查步骤**：

1. 检查 `mkdocs.yml` 语法是否正确（YAML 格式敏感）
2. 确保所有 `nav` 中引用的文件存在
3. 检查 Python 环境和已安装的包：
   ```bash
   pip list | grep mkdocs
   ```

### mermaid 图表不渲染

**症状**：文档中的 mermaid 代码块显示为原始代码

**解决方案**：

1. 确保已安装 `mkdocs-mermaid2-plugin`：
   ```bash
   pip install mkdocs-mermaid2-plugin
   ```
2. 检查 `mkdocs.yml` 中 `plugins` 包含 `mermaid2`
3. 确认 `extra_javascript` 引用了 `js/mermaid.min.js`
4. 确认 `pymdownx.superfences` 的 `custom_fences` 使用了 `mermaid2.fence_mermaid_custom` 格式

### 数学公式不渲染

**症状**：`$公式$` 或 `$$公式$$` 显示为原始文本

**解决方案**：

1. 检查 `docs/js/` 目录下是否有 `katex.min.js`、`auto-render.min.js`、`katex-init.js`
2. 检查 `docs/css/` 目录下是否有 `katex.min.css`
3. 确认 `mkdocs.yml` 的 `extra_javascript` 和 `extra_css` 引用了这些文件
4. 确认 `pymdownx.arithmatex` 配置了 `generic: true`

### 内网环境部署注意事项

本项目的 mermaid 图表和 KaTeX 公式均使用本地静态资源渲染，不依赖外网 CDN。部署时需确保：

- `docs/js/` 和 `docs/css/` 目录完整复制到部署目录中
- `mkdocs build` 会自动将这些文件包含在 `site/` 产物中
- 无需额外配置代理或镜像源

### 构建后样式丢失

**症状**：构建后的站点缺少样式或图表

**排查步骤**：

1. 检查配置文件中的路径引用是否正确
2. 确保文档中的图片和资源路径使用相对路径
3. 检查 `site/` 目录下是否包含 `js/` 和 `css/` 文件

### i18n 插件报错

**症状**：`i18n` 相关配置报错

**解决方案**：

```bash
# 安装 i18n 插件
pip install mkdocs-static-i18n
```

---

## 编写文档规范

项目文档遵循 `docs/doc-writing-guide.md` 中的撰写规范。关键要点：

- **语言**：中文（简体），UTF-8 编码
- **Admonition 语法**：使用 MkDocs Material 的 `!!!` 语法
  ```markdown
  !!! note "标题"
      内容
  
  !!! tip "提示"
      提示内容
  
  !!! warning "警告"
      警告内容
  ```
- **代码块**：必须指定语言标识，关键行添加中文注释
- **图表**：使用 mermaid 语法，每个图表前后添加文字说明
- **交叉引用**：使用相对路径 `[文本](./other-doc.md)`

详细规范参见 [文档撰写指南](./doc-writing-guide.md)。
