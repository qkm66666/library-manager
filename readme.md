# 图书管理系统

中山大学2024学年下学期软件工程第二次个人作业

使用 deepseek 和 GPT4o/Github Copilot 辅助实现的 Web 端图书管理系统，拥有图书增删改查、标签分类和数目统计等功能。

本项目使用 [crow](https://github.com/CrowCpp/Crow) 作为 HTTP 框架，并结合 SQLite 数据库，处理增删改查操作。

## 主要文件
- [server.cpp](server.cpp)：后端服务入口，提供图书增删改查的 API。
- [index.html](index.html)：前端界面，展示和管理图书列表。

## 编译与运行
1. 使用 CMake 配置并生成项目文件。  
2. 运行生成的可执行文件，监听在 8080 端口。

## 依赖
- Crow  
- SQLite3  

欢迎提交意见或建议！