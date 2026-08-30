---
title: DAWorkBench — 基于工作流的数据分析平台
description: C++/Qt 工作流引擎 + 内嵌 Python (pandas) + 交互式图表 + AI 驱动分析，为科研与工程数据处理打造的一站式桌面平台。
hide:
  - navigation
  - toc
---

<div class="da-hero">
<div class="da-hero-bg"></div>
<div class="da-hero-inner">

<p class="da-hero-badge">开源 &middot; LGPL 协议</p>

<h1 class="da-hero-title">DAWorkBench</h1>

<p class="da-hero-desc">
基于工作流的数据分析平台：有向图工作流引擎驱动自动化处理，GUI 封装 pandas 核心功能，
交互式图表生成论文级矢量图，集成 AI Agent 实现自然语言驱动分析。
</p>

<div class="da-hero-pills">
<span>C++17</span>
<span>Qt 5.14+ / Qt 6</span>
<span>Python &middot; pandas</span>
<span>pybind11</span>
<span>LangGraph</span>
</div>

<div class="da-hero-actions">
<a href="quick-start/" class="da-btn da-btn-primary">快速上手</a>
<a href="use-guide/" class="da-btn da-btn-ghost">使用指南</a>
<a href="https://github.com/czyt1988/data-workbench" class="da-btn da-btn-ghost">GitHub</a>
</div>

</div>
</div>

<div class="da-stats-bar">
<div class="da-stat">
<strong>20</strong>
<span>Agent 内置工具</span>
</div>
<div class="da-stat-divider"></div>
<div class="da-stat">
<strong>Qt 5 &amp; 6</strong>
<span>双版本兼容</span>
</div>
<div class="da-stat-divider"></div>
<div class="da-stat">
<strong>Win &amp; Linux</strong>
<span>跨平台</span>
</div>
<div class="da-stat-divider"></div>
<div class="da-stat">
<strong>LGPL</strong>
<span>商业友好</span>
</div>
</div>

<div class="da-container">

<div class="da-section-head">
<h2>核心特性</h2>
<p>为科研数据分析、一维仿真建模与工业数据处理提供一站式解决方案。</p>
</div>

<div class="da-card-grid">

<div class="da-card">
<div class="da-card-icon">
<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="18" cy="5" r="3"></circle><circle cx="6" cy="12" r="3"></circle><circle cx="18" cy="19" r="3"></circle><line x1="8.59" y1="13.51" x2="15.42" y2="17.49"></line><line x1="15.41" y1="6.51" x2="8.59" y2="10.49"></line></svg>
</div>
<h3>工作流驱动</h3>
<p>有向图描述数据处理流程，每个步骤封装为可配置节点。一次设计流程，一键重复执行，告别重复性手动处理。</p>
</div>

<div class="da-card">
<div class="da-card-icon">
<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"></polyline></svg>
</div>
<h3>Python 数据处理</h3>
<p>GUI 封装 pandas / numpy / scipy 核心功能，无需编写代码即可操作 DataFrame，并支持自定义 Python 脚本节点灵活扩展。</p>
</div>

<div class="da-card">
<div class="da-card-icon">
<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="20" x2="18" y2="10"></line><line x1="12" y1="20" x2="12" y2="4"></line><line x1="6" y1="20" x2="6" y2="14"></line></svg>
</div>
<h3>论文级可视化</h3>
<p>交互式图表编辑，拖拽调整元素位置、实时预览效果，支持导出 SVG / PDF 矢量图，直接用于论文插图。</p>
</div>

<div class="da-card">
<div class="da-card-icon">
<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="12 2 2 7 12 12 22 7 12 2"></polygon><polyline points="2 17 12 22 22 17"></polyline><polyline points="2 12 12 17 22 12"></polyline></svg>
</div>
<h3>插件化架构</h3>
<p>核心功能与业务逻辑分离，内置数据分析、系统节点、Agent 工具三类插件，提供完整的插件开发接口与生命周期管理。</p>
</div>

<div class="da-card">
<div class="da-card-icon">
<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="4" width="16" height="16" rx="2"></rect><rect x="9" y="9" width="6" height="6"></rect><line x1="9" y1="1" x2="9" y2="4"></line><line x1="15" y1="1" x2="15" y2="4"></line><line x1="9" y1="20" x2="9" y2="23"></line><line x1="15" y1="20" x2="15" y2="23"></line><line x1="20" y1="9" x2="23" y2="9"></line><line x1="20" y1="14" x2="23" y2="14"></line><line x1="1" y1="9" x2="4" y2="9"></line><line x1="1" y1="14" x2="4" y2="14"></line></svg>
</div>
<h3>AI 驱动分析</h3>
<p>多供应商 LLM 接入、内置提示词库与 20 个工作区工具，自然语言驱动数据分析；Agent 会话随工程文件保存与恢复。</p>
</div>

<div class="da-card">
<div class="da-card-icon">
<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="3" width="20" height="14" rx="2" ry="2"></rect><line x1="8" y1="21" x2="16" y2="21"></line><line x1="12" y1="17" x2="12" y2="21"></line></svg>
</div>
<h3>跨平台桌面应用</h3>
<p>兼容 Qt 5.14+ 与 Qt 6，支持 Windows 与 Linux，CMake 统一构建体系，Ribbon 界面 + 可停靠面板的原生桌面体验。</p>
</div>

</div>
</div>

<div class="da-showcase">

<div class="da-section-head">
<h2>界面展示</h2>
<p>从工作流编辑到 AI 对话分析，点击截图进入对应功能文档。</p>
</div>

<div class="da-gallery">

<a class="da-gallery-item" href="use-guide/">
<img src="../assets/screenshot/01.png" alt="DAWorkBench 主界面" loading="lazy">
<span>主界面</span>
</a>

<a class="da-gallery-item" href="use-guide/data-management/">
<img src="../assets/screenshot/02.png" alt="数据分析界面" loading="lazy">
<span>数据分析</span>
</a>

<a class="da-gallery-item" href="dev-guide/agent/">
<img src="../assets/screenshot/agent-analysis.gif" alt="Agent 对话分析演示" loading="lazy">
<span>AI 对话分析</span>
</a>

<a class="da-gallery-item" href="use-guide/chart-usage/">
<img src="../assets/screenshot/agent-auto-create-chart.gif" alt="Agent 自动生成图表演示" loading="lazy">
<span>Agent 自动绘图</span>
</a>

</div>

</div>

<div class="da-quickstart" markdown>

<div class="da-quickstart-inner" markdown>

<div class="da-section-head">
<h2>快速开始</h2>
<p>几分钟内完成环境搭建并运行 DAWorkBench。</p>
</div>

=== "克隆与第三方库"

    ```bash
    # 克隆仓库（必须使用 git clone，第三方库以 submodule 管理）
    git clone https://github.com/czyt1988/data-workbench.git
    cd data-workbench

    # 拉取第三方库
    git submodule update --init --recursive
    ```

=== "Windows 构建"

    ```powershell
    # 配置项目（必须指定 Qt 工具链文件）
    cmake -S . -B build -G Ninja `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_TOOLCHAIN_FILE="D:\Qt\6.7.3\msvc2019_64\lib\cmake\Qt6\qt.toolchain.cmake"

    # 构建并安装
    cmake --build build --config Release --parallel
    cmake --build build --config Release --target install
    ```

=== "Linux 构建"

    ```bash
    # 配置项目（按实际 Qt 安装路径修改工具链文件）
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/Qt/6.7.3/gcc_64/lib/cmake/Qt6/qt.toolchain.cmake

    # 构建并安装
    cmake --build build --config Release --parallel
    cmake --build build --config Release --target install
    ```

</div>
</div>

<div class="da-footer">
<div class="da-footer-inner">

<h2>开始你的数据分析工作流</h2>
<p>浏览使用指南，或深入了解插件与 Agent 开发。</p>

<div class="da-footer-links">
<a href="quick-start/" class="da-btn da-btn-primary">快速上手</a>
<a href="plugin/plugin-development/" class="da-btn da-btn-ghost">插件开发</a>
<a href="dev-guide/agent/" class="da-btn da-btn-ghost">Agent 开发</a>
<a href="https://github.com/czyt1988/data-workbench" class="da-btn da-btn-ghost">GitHub</a>
</div>

</div>
</div>
