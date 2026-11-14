cmake_minimum_required(VERSION 3.26)
project(h5pp-add-subdirectory LANGUAGES C CXX)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../.." h5pp)

add_executable(h5pp-add-subdirectory ../find-package/main.cpp)
target_link_libraries(h5pp-add-subdirectory PRIVATE h5pp::h5pp)

enable_testing()
add_test(NAME h5pp-add-subdirectory COMMAND h5pp-add-subdirectory)
