# python环境配置

## 构建过程python环境配置

`data-workbench`可以不依赖Python，通过配置`CMakeLists.txt`的`DA_ENABLE_PYTHON`选项可以禁用python环境，此时将不会编译python相关模块（如果你仅仅只是为了使用绘图模块，可以把python环境禁用）

如果开启将自动查找系统的python环境并进行依赖

## python环境要求和依赖说明

python环境有如下要求：

- 至少是python3.7

python环境需要安装的库：

```shell
pip install -r "{DataWorkBench_Dir}/requirements.txt"
```

> {DataWorkBench_Dir}是data-workbench的根目录

下面是`data-workbench`依赖的python库说明

- Loguru主要用于进行python脚本的日志记录
- openpyxl是pandas导入excel文件的依赖，如果没有安装，则无法导入excel文件
- chardet主要用于检测字符编码
- PyWavelets是进行小波分析的库
- pyarrow是dataframe进行扩展数据导入的库，支持parquet（Partitioning Parquet files）和Feather

python在win10~11操作系统下，安装package会把包安装到`用户名\AppData\Roaming\Python\Pythonxx\site-packages`下，这对于把整个python打包是不利的，因此需要在执行pip install时加上参数`--target`指定安装路径，例如：

```shell
pip install --target="../Lib/site-packages" -r "{DataWorkBench_Dir}/requirements.txt"
```

## python选择

这里建议使用python3.11版本，后续的ai操作基本都要求3.11以上版本

建议使用`Windows embeddable package`，windows版本下载地址：[https://www.python.org/downloads/windows](https://www.python.org/downloads/windows)

`embeddable package`是专门为了嵌入程序准备的，不需要安装到系统环境，只需要解压到程序安装目录下即可。

但对于开发来说，需要引入Python的lib文件和头尾文件，因此非embeddable版本也是必须的

你的电脑应该已经安装了python3.xx版本，这个版本是你平时使用的python环境。但对于data-workbench来说，你的环境有很多没有必要的库，不需要打包进来，为了真正发布能有一个相对干净的python环境，建议使用你当前电脑同样版本的`embeddable package`

这样，你可以在程序中使用python的库，而不会影响到系统的python环境。你系统的python环境安装的其它库也不会在打包时带入程序中

因此你的系统应该有如下两个python环境：

- 系统python环境，3.xx版本
- 需要打包到data-workbench的python环境，3.xx版本，用于打包python相关模块

这两个版本一定要完全匹配，包括版本号、架构

### 配置embeddable package方法

`embeddable package`是一个独立的python环境，不需要安装到系统环境，只需要解压到程序安装目录下即可，但缺乏pip，需要进行如下设置

#### 步骤一：修改embeddable package配置文件

1. 在Python解压目录下，找到名为 `python3xx._pth` 的文件（其中 `xx` 是版本号，例如 `python313._pth`）。
2. 用**记事本**打开它。
3. 找到 `#import site` 这一行，**删除行首的 `#`**，使其变为 `import site`。
4. **保存并关闭文件**。

#### 步骤二：安装pip工具

1. **下载安装脚本**：在浏览器中打开 `https://bootstrap.pypa.io/get-pip.py`，将网页内容**另存为** `get-pip.py` 文件，并保存到你的Python解压目录下。

2. **打开命令行并安装**：
    - 在Python目录的空白处，按住 `Shift` 键并点击鼠标右键，选择“在此处打开PowerShell窗口”或“打开命令窗口”。
    - 在弹出的窗口中输入以下命令并回车：

       ```bash
       .\python.exe get-pip.py
       ```

    - 等待安装完成，pip 就会被成功安装。
    - 安装完成后可以删除`get-pip.py`文件。

#### 步骤三：安装setuptools 和 wheel

需要先安装 `setuptools` 和 `wheel` 这两个基础工具

```shell
.\python.exe -m pip install --target="./Lib/site-packages" setuptools wheel
```

#### 安装所有依赖

现在，你可以像往常一样使用pip来安装第三方库了。关键是在所有pip命令前，都要加上 `.\python.exe -m` 前缀，以确保命令是在这个嵌入式环境中执行。

```bash
.\python.exe -m pip install --target="./Lib/site-packages" -r "{DataWorkBench_Dir}/requirements.txt"
```

如果速度慢，你可以加上清华的镜像源或其它镜像源，例如：

```bash
.\python.exe -m pip install --target="./Lib/site-packages" -r "{DataWorkBench_Dir}/requirements.txt" -i https://pypi.tuna.tsinghua.edu.cn/simple
```

#### 完善库路径

为了避免库路径查找异常，把python3xx.zip的内容解压到Lib目录下，形成一个完整的python环境。

或者把Lib/site-packages目录添加到python3xx.zip中

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

1. 如果没有`python-config.json`文件，将使用`where python`来查找系统的python环境
