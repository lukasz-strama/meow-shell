cmake_minimum_required(VERSION 3.0)

project(MeowShell LANGUAGES CXX)

set(SOURCE src/main.c++)

add_executable(${PROJECT_NAME} ${SOURCE})
