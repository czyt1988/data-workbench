# 程序命令参数

程序支持命令行参数，通过命令行参数可以控制程序运行时的行为。

## 参数一览

| 参数 | 说明 |
|------|------|
| `--help` / `-h` | 显示帮助信息 |
| `--version` / `-v` | 显示程序版本号 |
| `[project]` | 打开指定的工程文件（位置参数） |
| `--import-data <path>` | 导入数据文件 |

## 打开工程文件

可以直接在命令行指定要打开的工程文件路径（`.dapro` 文件），程序启动后会自动加载该工程：

```shell
DAWorkBench.exe "D:\projects\my-project.dapro"
```

## --import-data

导入数据文件。支持的文件格式包括：CSV、XLSX、TXT、PKL（Python pickle）等。

示例：

```shell
DAWorkBench.exe --import-data "D:\data.csv"
```

如果需要导入多个数据文件，可以使用多次 `--import-data` 参数：

```shell
DAWorkBench.exe --import-data "D:\data1.csv" --import-data "D:\data2.csv"
```

也可以同时打开工程文件并导入数据：

```shell
DAWorkBench.exe "D:\my-project.dapro" --import-data "D:\data.csv"
```

## --version

显示程序版本号并退出：

```shell
DAWorkBench.exe --version
```

## --help

显示帮助信息并退出：

```shell
DAWorkBench.exe --help
```
