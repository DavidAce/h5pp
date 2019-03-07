cmake_minimum_required(VERSION 3.10)
project(helloworld)
add_executable(${PROJECT_NAME} helloworld.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE h5pp deps)
