#include <iostream>
#include <cstdlib>
#include <thread>
#include <csignal>
#include <windows.h>

// 全局变量，用于控制后端进程
PROCESS_INFORMATION serverProcessInfo;

// 信号处理函数
void handleSignal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "收到终止信号，正在关闭后端服务器..." << std::endl;

        // 终止后端进程
        TerminateProcess(serverProcessInfo.hProcess, 0);
        CloseHandle(serverProcessInfo.hProcess);
        CloseHandle(serverProcessInfo.hThread);

        std::cout << "后端服务器已关闭。" << std::endl;
        exit(0);
    }
}

// 启动后端服务器
void startBackend() {
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    ZeroMemory(&serverProcessInfo, sizeof(PROCESS_INFORMATION));

    // 启动后端进程
    if (!CreateProcess(
            "build/Debug/LibraryManager.exe",   // 后端可执行文件路径
            NULL,           // 命令行参数
            NULL,           // 进程安全属性
            NULL,           // 线程安全属性
            FALSE,          // 是否继承句柄
            0,              // 创建标志
            NULL,           // 环境变量
            NULL,           // 工作目录
            &si,            // 启动信息
            &serverProcessInfo)) { // 进程信息
        std::cerr << "无法启动后端服务器: " << GetLastError() << std::endl;
        exit(1);
    }

    std::cout << "后端服务器已启动。" << std::endl;
}

// 启动前端页面
void startFrontend() {
    // 使用默认浏览器打开前端页面
    system("start index.html");
    std::cout << "前端页面已启动。" << std::endl;
}

int main() {
    // 设置控制台编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    

    // 注册信号处理器
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    // 启动后端和前端
    startBackend();
    startFrontend();

    // 主线程等待终止信号
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}