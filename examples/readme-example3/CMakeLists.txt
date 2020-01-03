cmake_minimum_required(VERSION 3.10)
project(readme-example3)
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE h5pp)


