cmake_minimum_required(VERSION 3.12)
project(helloworld)
add_executable(${PROJECT_NAME} helloworld.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE h5pp)
if (MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")
endif ()

