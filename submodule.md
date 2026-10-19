# submodule

本项目的第三方库用 submodule 管理。

第三方库的构建见：[https://czyt1988.github.io/data-workbench/zh/build/third-party-build](https://czyt1988.github.io/data-workbench/zh/build/third-party-build)

第三方库的管理与介绍见：[https://czyt1988.github.io/data-workbench/zh/dev-guide/third-party-manage](https://czyt1988.github.io/data-workbench/zh/dev-guide/third-party-manage)

## submodule 的拉取

首次拉取项目之后，需要执行：

```shell
git submodule update --init --recursive
```

这条命令拉取全部 submodule。

也可以逐个拉取：

```shell
git submodule update src/3rdparty/DAWidgets
git submodule update src/3rdparty/zlib
git submodule update src/3rdparty/quazip
git submodule update src/3rdparty/spdlog
git submodule update src/3rdparty/SARibbon
git submodule update src/3rdparty/ADS
git submodule update src/3rdparty/pybind11
git submodule update src/3rdparty/QtPropertyBrowser  # 已停用，源码保留但不参与构建
git submodule update src/3rdparty/ordered-map
```

## submodule 的添加

第三方库统一放在 `/src/3rdparty/xxx` 目录下。

添加 submodule 的方法以 pybind11 为例：

`git submodule add https://github.com/pybind/pybind11 ./src/3rdparty/pybind11`

- 最后的路径必须是 `./src/3rdparty/pybind11`，库名称文件夹要写出来。

项目当前添加的 submodule 如下。

github 源：

```shell
git submodule add https://github.com/madler/zlib ./src/3rdparty/zlib
git submodule add https://github.com/stachenov/quazip ./src/3rdparty/quazip
git submodule add https://github.com/gabime/spdlog ./src/3rdparty/spdlog
git submodule add https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System ./src/3rdparty/ADS
git submodule add https://github.com/czyt1988/SARibbon ./src/3rdparty/SARibbon
git submodule add https://github.com/czyt1988/DAWidgets ./src/3rdparty/DAWidgets
git submodule add https://github.com/pybind/pybind11 ./src/3rdparty/pybind11
git submodule add https://github.com/czyt1988/QtPropertyBrowser ./src/3rdparty/QtPropertyBrowser
git submodule add https://github.com/czyt1988/QWT ./src/3rdparty/qwt
git submodule add https://github.com/Tessil/ordered-map ./src/3rdparty/ordered-map
```

gitee 源：
```shell
git submodule add https://gitee.com/czyt1988/zlib ./src/3rdparty/zlib
git submodule add https://gitee.com/czyt1988/quazip ./src/3rdparty/quazip
git submodule add https://gitee.com/czyt1988/spdlog ./src/3rdparty/spdlog
git submodule add https://gitee.com/czyt1988/Qt-Advanced-Docking-System ./src/3rdparty/ADS
git submodule add https://gitee.com/czyt1988/SARibbon ./src/3rdparty/SARibbon
git submodule add https://gitee.com/czyt1988/DAWidgets ./src/3rdparty/DAWidgets
git submodule add https://gitee.com/czyt1988/pybind11 ./src/3rdparty/pybind11
git submodule add https://gitee.com/czyt1988/QtPropertyBrowser ./src/3rdparty/QtPropertyBrowser
git submodule add https://gitee.com/czyt1988/QWT ./src/3rdparty/qwt
git submodule add https://gitee.com/czyt1988/ordered-map ./src/3rdparty/ordered-map
```
## submodule 的更新

某个 submodule 更新之后，用 `git submodule update --remote {submodule}` 拉取新版本。比如 SARibbon 更新了：

```shell
git submodule update --remote src/3rdparty/SARibbon
```

## submodule 的批量操作

进入某个 submodule 目录后，git 命令和普通仓库一样用。批量执行时：

`git submodule foreach '{需要执行的git命令}'`

比如 `git submodule foreach 'git checkout main'`

## 替换为 github 源

受 github 访问限制，项目默认的 submodule 地址都是 gitee 的。切换成 github 源按下面两步执行。

1. 调整 `.gitmodules` 文件

	`.gitmodules` 里写着每个第三方库的地址，改成 github 地址即可。下面是 github 源的写法：

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

	改成 gitee 地址时写法如下：

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

2. 同步调整 `./.git/config` 里对应的 url

	只改 `.gitmodules` 不生效，`.git/config` 里对应的 url 也要一起改。
