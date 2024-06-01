# DA 工程文件结构说明

## 顶层节点<root>

工程文件以xml形式保存顶层节点为<root>,节点属性`type`用于标记此xml的类型，目前可选项为：

- project 代表这是工程文件

## 本地信息记录<local-info>

<local-info>位于<root>节点下，用于保存保存文件的本地计算机信息

- <machineHostName> 计算机名
- <cpuArch> cpu信息
- <kernelType> 操作系统类型
- <kernelVersion> 操作系统版本
- <prettyProductName> 操作系统全称