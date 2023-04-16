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