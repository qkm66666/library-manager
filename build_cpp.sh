#!/bin/bash
# filepath: d:\git_project\library-manager\#!/bin/bash

# Stop on first error
set -e

# ============================================================================
#                            CONFIGURATION
# ============================================================================
# Set the absolute path to your vcpkg installation directory. Use forward slashes.
VCPKG_ROOT="D:/vcpkg"

# Get the project root directory (where this script is located).
PROJECT_ROOT=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# ============================================================================
#                            SANITY CHECKS
# ============================================================================
echo "Checking for vcpkg..."
if [ ! -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
    echo "ERROR: vcpkg.cmake not found at the specified VCPKG_ROOT."
    echo "Please edit this script and set the VCPKG_ROOT variable correctly."
    exit 1
fi
echo "vcpkg found."

echo "Checking for Python..."
if ! command -v python &> /dev/null; then
    echo "WARNING: Python executable not found in PATH."
    echo "The frontend server will not be started."
    echo "Please install Python and add it to your PATH to serve the frontend."
fi
echo "Python found."


# ============================================================================
#                         BACKEND COMPILATION
# ============================================================================
echo
echo "==================================="
echo "     COMPILING BACKEND (C++)"
echo "==================================="
echo

BUILD_DIR="$PROJECT_ROOT/build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

echo "Configuring project with CMake..."
cmake .. -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

echo "Building project..."
cmake --build . --config Release

echo "Build successful."

# ============================================================================
#                         STARTING SERVERS
# ============================================================================
echo
echo "==================================="
echo "        STARTING SERVERS"
echo "==================================="
echo

# Start Backend Server in the background
BACKEND_EXE="$BUILD_DIR/Release/LibraryManager.exe"
if [ -f "$BACKEND_EXE" ]; then
    echo "Starting Backend server..."
    "$BACKEND_EXE" &
    BACKEND_PID=$!
    echo "Backend server started with PID $BACKEND_PID."
else
    echo "ERROR: Backend executable not found at $BACKEND_EXE."
    exit 1
fi

# Start Frontend Server in the background (if Python is available)
if command -v python &> /dev/null; then
    echo "Starting Frontend server on http://localhost:8000"
    cd "$PROJECT_ROOT"
    python -m http.server 8000 &
    FRONTEND_PID=$!
    echo "Frontend server started with PID $FRONTEND_PID."
fi

echo
echo "All servers are running in the background."
echo "You can close this terminal, but the servers will stop."
echo "To stop the servers, close this terminal or use 'kill $BACKEND_PID $FRONTEND_PID'"

# Keep the script running to show the PIDs, or just exit
wait


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