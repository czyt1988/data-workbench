# 第三方库构建过程

第三方库构建需要按照下面步骤进行：

1. 独立构建库（zlib），并安装
2. cmake打开`./src/3rdparty/CMakeLists.txt`对剩下的第三方库进行构建

> zlib无法直接集成到`./src/3rdparty/CMakeLists.txt`中，因此需要单独构建，并且安装，能通过find_package找到，如果不能找到，需要单独指定zlib的路径（设置zlib_DIR）

# 常见问题

- 第一次构建时出错

看到错误的详细信息是moc出错，这时你可以再次构建，一般就能成功，这个主要是qt的moc程序的bug，大量项目进行moc时，有时会异常退出