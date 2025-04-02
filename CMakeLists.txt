# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(LibraryManager)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(D:/vcpkg/installed/x64-windows/include)
link_directories(D:/vcpkg/installed/x64-windows/lib)


# Add source files
set(SOURCES
    server.cpp
)

# Add executable target
add_executable(LibraryManager ${SOURCES})

# Use vcpkg to find Crow and OpenSSL
find_package(Crow CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# Link Crow, OpenSSL, and SQLite3
target_link_libraries(LibraryManager PRIVATE Crow::Crow OpenSSL::SSL OpenSSL::Crypto sqlite3)

# Enable warnings for better code quality
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(LibraryManager PRIVATE -Wall -Wextra -pedantic)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(LibraryManager PRIVATE /W4)
endif()