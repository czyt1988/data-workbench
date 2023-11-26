mkdir build
cd build
cmake -A x64 ..
cmake --build .
cmake --install .
#  使用read命令达到类似bat中的pause命令效果
echo 按任意键继续
read -n 1
echo 继续运行