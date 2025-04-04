#!/bin/bash
# filepath: d:\git_project\library-manager\build.sh

# 创建新的构建目录
cd build

# 运行 CMake 生成构建文件
cmake ..

# 编译项目
cmake --build . --config Debug

# 返回到项目根目录
cd ..

# 启动后端
build/Debug/LibraryManager.exe &

# 等待后端初始化
sleep 1

# 启动前端
start ./index.html