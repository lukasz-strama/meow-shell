cmake_minimum_required(VERSION 3.0)

project(meow-shell)

set(CMAKE_CXX_STANDARD)

set(SOURCE src/main.c++)

add_executable(${PROJECT_NAME} ${SOURCE})
