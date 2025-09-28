# python环境配置

## 构建过程python环境配置

`data-workbench`可以不依赖Python，通过配置`CMakeLists.txt`的`DA_ENABLE_PYTHON`选项可以禁用python环境，此时将不会编译python相关模块（如果你仅仅只是为了使用绘图模块，可以把python环境禁用）

如果开启将自动查找系统的python环境并进行依赖

## python环境要求和依赖说明

python环境有如下要求：

- 至少是python3.7

python环境需要安装的库：

```
pip install Loguru numpy==1.26.4 pandas scipy openpyxl chardet PyWavelets pyarrow
```

如果你的python版本小于3.8，还需额外导入`typing_extensions`库

```
pip install typing_extensions
```

为了兼容性，numpy建议2.0以下，这里推荐使用1.26.4

下面是`data-workbench`依赖的python库说明

- Loguru主要用于进行python脚本的日志记录
- openpyxl是pandas导入excel文件的依赖，如果没有安装，则无法导入excel文件
- chardet主要用于检测字符编码
- PyWavelets是进行小波分析的库
- pyarrow是dataframe进行扩展数据导入的库，支持parquet（Partitioning Parquet files）和Feather

python在win11操作系统下，安装package会把包安装到`用户名\AppData\Roaming\Python\Pythonxx\site-packages`下，这对于把整个python打包是不利的，因此需要在执行pip install时加上参数`--target`指定安装路径，例如：

```shell
pip install --target="../Lib/site-packages" loguru numpy==1.26.4 pandas scipy openpyxl chardet PyWavelets pyarrow
```

项目目录下已经把所有依赖放到了`requirements.txt`文件里，可以直接使用`pip install -r requirements.txt`进行安装

## 程序运行时查找python逻辑

dataworkbench查找python的逻辑是：

1. 先查看程序运行目录下是否存在`python-config.json`，如果有，讲读取python-config.json里的`config/interpreter`下的值，以此作为python解析器的路径,python-config.json的模板如下：

```json
{
  "config": {
      "interpreter": "path to python interpreter"
    }
}
```

> 程序安装目录可以使用`${current-app-dir}`变量替代，例如python安装在程序安装目录下，那么`${current-app-dir}`的值就是程序安装目录，如：${current-app-dir}/python311/python.exe

2. 如果没有`python-config.json`文件，将使用`where python`来查找系统的python环境