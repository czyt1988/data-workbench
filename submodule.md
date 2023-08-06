# submodule

本项目的第三方库使用submodule进行管理

## submdule的拉取

首次拉取项目之后，需要执行：

```shell
git submodule update --init --recursive
```

对所有的submodule进行拉取

当然也可以逐个拉取：

```shell
git submodule update src/3rdparty/spdlog
git submodule update src/3rdparty/SARibbon
git submodule update src/3rdparty/Qt-Advanced-Docking-System
git submodule update src/3rdparty/pybind11
```

## submodule的添加

第三方库统一放置到`/src/3rdparty`目录下，因此添加submodule如下：

`git submodule add https://gitee.com/czyt1988/pybind11.git ./src/3rdparty/pybind11`

## submodule的更新

如果某个submodule更新了，使用`git submodule update --remote {submodule}`进行更新，如SARibbon项目更新了，可以执行：

```shell
git submodule update --remote src/3rdparty/SARibbon
```

## submodule的批量操作

单独进入某个submodule目录下执行git命令和普通的git仓库操作一样，也可以批量执行，如

`git submodule foreach '{需要执行的git命令}'`

如 git submodule foreach 'git checkout main'

# 针对中国地区访问github缓慢的问题

用编辑器打开

```
.git/config
```

把config的github地址替换为gitee的地址

github地址

```ini
[submodule "src/3rdparty/Qt-Advanced-Docking-System"]
	active = true
	url = https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git
[submodule "src/3rdparty/SARibbon"]
	active = true
	url = https://github.com/czyt1988/SARibbon.git
[submodule "src/3rdparty/pybind11"]
	active = true
	url = https://github.com/pybind/pybind11.git
[submodule "src/3rdparty/spdlog"]
	active = true
	url = https://github.com/gabime/spdlog.git
```

gitee地址

```ini
[submodule "src/3rdparty/Qt-Advanced-Docking-System"]
	active = true
	url = https://gitee.com/czyt1988/Qt-Advanced-Docking-System.git
[submodule "src/3rdparty/SARibbon"]
	active = true
	url = https://gitee.com/czyt1988/SARibbon.git
[submodule "src/3rdparty/pybind11"]
	active = true
	url = https://gitee.com/czyt1988/pybind11.git
[submodule "src/3rdparty/spdlog"]
	active = true
	url = https://gitee.com/czyt1988/spdlog.git
```

同时把根目录的`.gitmodules`用`.gitmodules-gitee`的内容替代