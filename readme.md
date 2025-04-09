# 图书管理系统

中山大学2024学年下学期软件工程第二次个人作业

使用 deepseek 和 GPT4o/Github Copilot 辅助实现的 Web 端图书管理系统，拥有图书增删改查、标签分类和数目统计等功能。

本项目使用 [crow](https://github.com/CrowCpp/Crow) 作为 HTTP 框架，并结合 SQLite 数据库，处理增删改查操作。

## 主要文件
- [server.cpp](server.cpp)：后端服务入口，提供图书增删改查的 API。
- [index.html](index.html)：前端界面，展示和管理图书列表。

## 编译与运行
### 使用 cmake 进行构建
1. 使用 CMake 配置并生成项目文件。
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
2. 运行生成的可执行文件，监听在 8080 端口。
3. 使用浏览器打开 index.html。

也可使用项目中提供的启动器 launcher.exe 来启动前后端。

### 使用 bash 脚本进行构建
使用本方法构建项目，需要确保已经正确安装 bash 并设置环境变量。

使用以下指令进行项目构建：
```bash
bash build.sh
```

运行脚本后将会自动调用 cmake 构建项目并启动。

## 依赖
- Crow  
- OpenSSL
- SQLite3  

欢迎提交意见或建议！