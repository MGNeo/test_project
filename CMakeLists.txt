cmake_minimum_required(VERSION 3.11)

project(test_project)

find_package(Threads REQUIRED)

file(GLOB HEADERS *.h)
file(GLOB SOURCES *.c)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} Threads::Threads)

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)

