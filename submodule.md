# submodule

本项目的第三方库使用submodule进行管理

## submdule的拉取

首次拉取项目之后，需要执行：

`git submodule update --init --recursive`

对所有的submodule进行拉取

## submodule的添加

第三方库统一放置到`/src/3rdparty`目录下，因此添加submodule如下：

git submodule add https://gitee.com/czyt1988/pybind11.git ./src/3rdparty/pybind11