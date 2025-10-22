# 构建过程常见错误

## vs2017译器构建出错

> error D8050: 无法执行 xxx/c1xx.dll  未能将命令行放入调试记录中

如果你的vs正确安装，但出现这个错误，有两种情况：你的构建目录可能存在中文，例如你的用户名就是中文，早期版本的vs，例如vs2017，会把构建目录放到用户的临时文件夹下面，这时就会导致构建出错，解决方法是定义
```json
{
  "configurations": [
    {
      "name": "x64-Debug",
      "generator": "Ninja",
      "configurationType": "Debug",
      "inheritEnvironments": [ "msvc_x64" ],
      "buildRoot": "${workspaceRoot}\\build\\x64-Debug",
      "cmakeCommandArgs": "",
      "ctestCommandArgs": ""
    },
    {
      "name": "x64-Release",
      "generator": "Ninja",
      "configurationType": "Release",
      "inheritEnvironments": [ "msvc_x64" ],
      "buildRoot": "${workspaceRoot}\\build\\x64-Release"
    }
  ]
}
```

另外还有一种情况就是你的操作系统最大路径没有放开，只支持255长度，这样非常容易出现问题，你可以通过修改组策略编辑器，把路径的最大长度设置为8192

```txt
按 Win + R 输入 gpedit.msc。

导航到：
计算机配置 > 管理模板 > 系统 > 文件系统
启用 启用 Win32 长路径。

重启系统。
```

如果还是不行，把项目移动到其他目录下，比如D盘，或者C盘，这样路径长度就变短了，这样问题就解决啦

## 构建过程中出现moc错误

在编译过程中可能会看到类似如下错误信息：

```txt
14:44:10: 为项目DAWorkbench执行步骤 ...
14:44:10: 正在启动 "D:\Qt\Tools\CMake_64\bin\cmake.exe" --build . --target all
...
[3/173 2.1/sec] Automatic MOC and UIC for target DAGui
FAILED: src/DAGui/DAGui_autogen/timestamp src/DAGui/DAGui_autogen/mocs_compilation.cpp F:/src/build-data-workbench-Desktop_Qt_6_4_0_MSVC2019_64bit-Debug/src/DAGui/DAGui_autogen/timestamp F:/src/build-data-workbench-Desktop_Qt_6_4_0_MSVC2019_64bit-Debug/src/DAGui/DAGui_autogen/mocs_compilation.cpp 
cmd.exe /C "cd .../CMakeFiles/d/8c503ea7614ae801d4e0edb644195bdd09d9cc85f654d6f7f478d372d1ca2271.d"
ninja: build stopped: subcommand failed.
14:44:12: 进程"D:\Qt\Tools\CMake_64\bin\cmake.exe"退出，退出代码 1 。
Error while building/deploying project DAWorkbench (kit: Desktop Qt 6.4.0 MSVC2019 64bit)
When executing step "Build"
14:44:12: Elapsed time: 00:02.
```

这种moc相关的错误，只需要再**多几次构建**即可，这个问题尤其容易发生在第一次构建的时候，大批量的moc操作有时会出现异常，只要保留build目录，继续构建即可

## mingw编译错误

mingw编译有时候会出现`not enough space for thread data`错误


```txt
[1/219 2.0/sec] Automatic MOC and UIC for target qwt
[2/219 1.4/sec] Automatic MOC and UIC for target SARibbonBar
[3/219 1.8/sec] Automatic MOC for target qt5advanceddocking
[4/219 2.2/sec] Automatic MOC and UIC for target DALiteCtk
[5/219 2.6/sec] Automatic MOC for target QuaZip
[6/218 2.8/sec] Automatic MOC and UIC for target QtPropertyBrowser

runtime error R6016
- not enough space for thread data
[7/218 1.2/sec] Building CXX object 
```

此错误处理方式和`构建过程中出现moc错误`处理方式一致，就是**再次或多次构建**就能解决，这一般是编译器自身的一些问题

## 编译完成运行报错

编译完成运行立即报错主要是你的构建目录下没有第三方库，编译完成后运行程序是在build目录下运行的，第一次构建的build目录下没有第三方的dll，因此一运行就会报错，你需要把第三方库dll都复制到build目录下

主要涉及的dll如下：

```
DALiteCtk.dll
qt6advanceddocking.dll
QtPropertyBrowser.dll
quazip1-qt6.dll
qwt.dll
SARibbonBar.dll
spdlog.dll
zlib.dll
```

!!! tips "提示"
    有些dll名字是和qt版本有关，例如`qt6advanceddocking.dll`和`quazip1-qt6.dll`是在Qt6编译的名字，如果在Qt5下编译，名字会变成`qt5advanceddocking.dll`和`quazip1-qt5.dll`

!!! warning "注意"
    zlib.dll是zlib库，它是quazip1-qt6.dll的依赖，这个库也需要手动复制到build目录下，否则无法运行

!!! warning "注意"
    如果你是debug模式构建，那么上诉所有库名字最后都会加上`d`，例如`SARibbonBar.dll`将变为`SARibbonBard.dll`

## 软件运行python报错

`data-workbench`依赖python，启动过程会寻找python，如果python环境没有指定，会使用`where python`命令获取操作系统下的python环境，如果找不到，会报错，也有可能找到了别的python环境，导致启动加载库失败，`data-workbench`对应的python库需要指定安装相关的包，具体见：[python环境配置](./python-environment.md)，你可以通过`python-config.json`让程序寻找指定的python环境



