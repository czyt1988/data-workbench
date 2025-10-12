# 第三方库管理

`data-workbench`的第三方库都放在`src\3rdparty`目录下，统一由`src\3rdparty\CMakeLists.txt`进行构建

如果你开发时想添加一个第三方库，以`pybind11`举例，你可以执行下面语句进行添加：

`git submodule add https://github.com/pybind/pybind11 ./src/3rdparty/pybind11`

!!! tips "注意"
    最后路径是`./src/3rdparty/pybind11`,库名称文件夹要指定

## 当前项目的第三方依赖

目前添加的submodule有如下：

| 第三方库目录                      | 仓库地址                                                          | 作用                        | 应用场景                            | 许可证                     |
| -------------------------------- | ---------------------------------------------------------------- | ---------------------------- | --------------------------------- | ----------------------- |
| `src/3rdparty/zlib`              | <https://github.com/madler/zlib>                                 | 通用无损压缩库（DEFLATE 算法事实标准）      | 打包/解包 `.zip`、减小网络传输体积、存储压缩        | zlib license            |
| `src/3rdparty/quazip`            | <https://github.com/stachenov/quazip>                            | Qt 封装层，让 Qt 程序轻松读写 `.zip` 文件 | 项目配置、资源包、自动更新模块的压缩/解压             | LGPL-2.1                |
| `src/3rdparty/spdlog`            | <https://github.com/gabime/spdlog>                               | 高性能header-only 日志库              | 运行时调试、性能埋点、本地化日志归档                | MIT                     |
| `src/3rdparty/ADS`               | <https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System> | Qt 高级停靠/浮动窗口框架               | IDE 式灵活布局、工具窗口拖拽停靠                | BSD-3-Clause            |
| `src/3rdparty/SARibbon`          | <https://github.com/czyt1988/SARibbon>                           | Qt  Ribbon 风格菜单栏（Office 样式）  | 提升大型桌面软件的外观与操作效率                  | MIT                     |
| `src/3rdparty/pybind11`          | <https://github.com/pybind/pybind11>                             | 把 C++ 代码无痛暴露给 Python 的轻量级绑定库 | 脚本化扩展、自动化测试、算法快速验证                | BSD-3-Clause            |
| `src/3rdparty/QtPropertyBrowser` | <https://github.com/czyt1988/QtPropertyBrowser>                  | Qt 属性表/属性编辑器控件               | 可视化修改对象属性、配置面板、序列化前端              | MIT                     |
| `src/3rdparty/qwt`               | <https://github.com/czyt1988/QWT>                                | 基于 Qt 的科学/工程图表与仪表盘库          | 实时曲线、频谱图、示波器、工业监控界面               | Qwt License (LGPL-like) |
| `src/3rdparty/ordered-map`       | <https://github.com/Tessil/ordered-map>                          | 保留插入顺序的哈希表 & 树形映射            | 需要“键值+顺序”双重语义的数据结构，如 JSON 编辑器、配置树 | MIT                     |


## submodule的更新

如果某个`submodule`更新了，使用`git submodule update --remote {submodule}`进行更新，如SARibbon项目更新了，可以执行：

```shell
git submodule update --remote src/3rdparty/SARibbon
```

## submodule的源管理

由于github访问域限制，目前项目默认的`subbmodule`的url都是gitee的，如果要切换为github的，可以按照如下步骤执行：

1. `.gitmodules`文件的调整

	打开`.gitmodules`文件，此文件包含了第三方库的地址，你可以对此地址进行调整，例如下面为当前第三方库的github地址配置

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

	你也可以调整为其它地址，例如

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
		url = https://gitee.com/czyt1988/zlib
		active = true
	```

2. 同步调整`./.git/config`文件里对应的url

	`.gitmodules`文件调整后需要同步调整`.git/config`文件里对应的url才能生效
