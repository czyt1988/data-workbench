# 第三方库管理

第三方库管理模块提供基于 Git Submodule 的依赖管理方案，支持 GitHub 和 Gitee 双源配置，统一构建入口简化开发流程。

## 主要功能特性

**特性**

- ✅ **Git Submodule 管理**：使用 Git submodule 机制管理第三方库版本
- ✅ **统一构建入口**：所有第三方库通过 `src/3rdparty/CMakeLists.txt` 统一构建
- ✅ **多源支持**：支持 GitHub 和 Gitee 双源配置，解决访问限制问题
- ✅ **版本更新机制**：提供标准的 submodule 更新命令

## 当前项目的第三方依赖

目前添加的 submodule 有如下：

| 第三方库目录 | 仓库地址 | 作用 | 应用场景 | 许可证 |
|-------------|---------|------|---------|-------|
| `src/3rdparty/zlib` | https://github.com/madler/zlib | 通用无损压缩库（DEFLATE 算法事实标准） | 打包/解包 `.zip`、减小网络传输体积、存储压缩 | zlib license |
| `src/3rdparty/quazip` | https://github.com/stachenov/quazip | Qt 封装层，让 Qt 程序轻松读写 `.zip` 文件 | 项目配置、资源包、自动更新模块的压缩/解压 | LGPL-2.1 |
| `src/3rdparty/spdlog` | https://github.com/gabime/spdlog | 高性能 header-only 日志库 | 运行时调试、性能埋点、本地化日志归档 | MIT |
| `src/3rdparty/ADS` | https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System | Qt 高级停靠/浮动窗口框架 | IDE 式灵活布局、工具窗口拖拽停靠 | BSD-3-Clause |
| `src/3rdparty/SARibbon` | https://github.com/czyt1988/SARibbon | Qt Ribbon 风格菜单栏（Office 样式） | 提升大型桌面软件的外观与操作效率 | MIT |
| `src/3rdparty/pybind11` | https://github.com/pybind/pybind11 | 把 C++ 代码无痛暴露给 Python 的轻量级绑定库 | 脚本化扩展、自动化测试、算法快速验证 | BSD-3-Clause |
| `src/3rdparty/QtPropertyBrowser` | https://github.com/czyt1988/QtPropertyBrowser | Qt 属性表/属性编辑器控件 | 可视化修改对象属性、配置面板、序列化前端 | MIT |
| `src/3rdparty/qwt` | https://github.com/czyt1988/QWT | 基于 Qt 的科学/工程图表与仪表盘库 | 实时曲线、频谱图、示波器、工业监控界面 | Qwt License (LGPL-like) |
| `src/3rdparty/ordered-map` | https://github.com/Tessil/ordered-map | 保留插入顺序的哈希表 & 树形映射 | 需要"键值+顺序"双重语义的数据结构，如 JSON 编辑器、配置树 | MIT |

## 添加第三方库

如果开发时想添加一个第三方库，以 `pybind11` 举例，可以执行下面语句进行添加。Git submodule 会自动更新 `.gitmodules` 文件：

```bash
git submodule add https://github.com/pybind/pybind11 ./src/3rdparty/pybind11
```

执行上述命令后，`pybind11` 仓库被克隆到 `src/3rdparty/pybind11` 目录，`.gitmodules` 文件自动添加对应配置。

!!! tip "注意"
    最后路径是 `./src/3rdparty/pybind11`，库名称文件夹要指定。

添加后的目录结构：

```txt
src/3rdparty/
├── CMakeLists.txt          # 统一构建入口
├── pybind11/               # 新添加的第三方库
│   ├── CMakeLists.txt
│   └── ...
└── ...
```

## 更新 Submodule

如果某个 submodule 更新了，使用 `git submodule update --remote` 进行更新。以下命令更新 SARibbon 到最新版本：

```bash
git submodule update --remote src/3rdparty/SARibbon
```

执行上述命令后，SARibbon submodule 被更新到远程仓库的最新提交。

### 更新所有 submodule

以下命令批量更新所有 submodule 到最新版本：

```bash
# 更新所有 submodule 到最新版本
git submodule update --remote

# 更新并合并到当前分支（自动处理冲突）
git submodule update --remote --merge
```

执行上述命令后，所有 submodule 被更新，第二个命令会自动合并远程变更到当前分支。

## Submodule 源管理

由于 GitHub 访问域限制，目前项目默认的 submodule url 都是 Gitee 的。如果要切换为 GitHub 的，可以按照如下步骤执行。

### 修改 `.gitmodules` 文件

打开 `.gitmodules` 文件，此文件包含了第三方库的地址，你可以对此地址进行调整。

GitHub 地址配置示例：

```ini
[submodule "src/3rdparty/spdlog"]
    url = https://github.com/gabime/spdlog.git
    active = true
[submodule "src/3rdparty/ADS"]
    url = https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git
    active = true
[submodule "src/3rdparty/SARibbon"]
    url = https://github.com/czyt1988/SARibbon.git
    active = true
[submodule "src/3rdparty/pybind11"]
    url = https://github.com/pybind/pybind11.git
    active = true
[submodule "src/3rdparty/QtPropertyBrowser"]
    url = https://github.com/czyt1988/QtPropertyBrowser.git
    active = true
[submodule "src/3rdparty/qwt"]
    url = https://github.com/czyt1988/QWT.git
    active = true
[submodule "src/3rdparty/ordered-map"]
    url = https://github.com/Tessil/ordered-map.git
    active = true
[submodule "src/3rdparty/quazip"]
    url = https://github.com/stachenov/quazip.git
    active = true
[submodule "src/3rdparty/zlib"]
    url = https://github.com/madler/zlib.git
    active = true
```

Gitee 地址配置示例：

```ini
[submodule "src/3rdparty/spdlog"]
    url = https://gitee.com/czyt1988/spdlog.git
    active = true
[submodule "src/3rdparty/ADS"]
    url = https://gitee.com/czyt1988/Qt-Advanced-Docking-System.git
    active = true
[submodule "src/3rdparty/SARibbon"]
    url = https://gitee.com/czyt1988/SARibbon.git
    active = true
[submodule "src/3rdparty/pybind11"]
    url = https://gitee.com/czyt1988/pybind11.git
    active = true
[submodule "src/3rdparty/QtPropertyBrowser"]
    url = https://gitee.com/czyt1988/QtPropertyBrowser.git
    active = true
[submodule "src/3rdparty/qwt"]
    url = https://gitee.com/czyt1988/QWT.git
    active = true
[submodule "src/3rdparty/ordered-map"]
    url = https://gitee.com/czyt1988/ordered-map.git
    active = true
[submodule "src/3rdparty/quazip"]
    url = https://gitee.com/czyt1988/quazip.git
    active = true
[submodule "src/3rdparty/zlib"]
    url = https://gitee.com/czyt1988/zlib.git
    active = true
```

### 同步配置到 `.git/config`

`.gitmodules` 文件调整后需要同步调整 `.git/config` 文件里对应的 url 才能生效：

```bash
# 同步 submodule 配置
git submodule sync
```

!!! warning "重要"
    修改 `.gitmodules` 后必须执行 `git submodule sync` 同步配置，否则更改不会生效。

## 常见问题

!!! bug "Submodule 目录为空"
    克隆项目后 submodule 目录为空，需要执行初始化：
    ```bash
    git submodule init
    git submodule update
    ```
    
    或者克隆时直接初始化：
    ```bash
    git clone --recurse-submodules https://github.com/xxx/data-workbench.git
    ```

!!! tip "批量操作"
    执行涉及 submodule 的批量操作时，可以使用 `--recursive` 参数：
    ```bash
    git status --recursive
    git fetch --recursive
    ```

## 参考资料

- [Git Submodule 官方文档](https://git-scm.com/book/en/v2/Git-Tools-Submodules)
- [QuaZip 使用指南](https://github.com/stachenov/quazip)
- [pybind11 文档](https://pybind11.readthedocs.io/)