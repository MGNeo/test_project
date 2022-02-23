cmake_minimum_required(VERSION 3.11)

project(test_project)

file(GLOB HEADERS *.h)
file(GLOB SOURCES *.c)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)

