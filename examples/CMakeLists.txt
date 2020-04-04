cmake_minimum_required(VERSION 3.12)
project(examples)
add_executable(example1 example1-writeDataset.cpp)
add_executable(example2 example2-readDataset.cpp)
add_executable(example3 example3-EigenMatrixXcd.cpp)
add_executable(example4 example4-attributes.cpp)
add_executable(example5 example5-customType-easy.cpp)
add_executable(example6 example6-customType-hard.cpp)
add_executable(example7 example7-HDF5-tables.cpp)

target_link_libraries(example1 PRIVATE h5pp)
target_link_libraries(example2 PRIVATE h5pp)
target_link_libraries(example3 PRIVATE h5pp)
target_link_libraries(example4 PRIVATE h5pp)
target_link_libraries(example5 PRIVATE h5pp)
target_link_libraries(example6 PRIVATE h5pp)
target_link_libraries(example7 PRIVATE h5pp)

if (MSVC)
    set_target_properties(example1 PROPERTIES LINK_FLAGS "/ignore:4099")
    set_target_properties(example2 PROPERTIES LINK_FLAGS "/ignore:4099")
    set_target_properties(example3 PROPERTIES LINK_FLAGS "/ignore:4099")
    set_target_properties(example4 PROPERTIES LINK_FLAGS "/ignore:4099")
    set_target_properties(example5 PROPERTIES LINK_FLAGS "/ignore:4099")
    set_target_properties(example6 PROPERTIES LINK_FLAGS "/ignore:4099")
    set_target_properties(example7 PROPERTIES LINK_FLAGS "/ignore:4099")
endif ()

# A handy target "all-examples" to build all examples
add_custom_target(build-all-examples DEPENDS
        example1
        example2
        example3
        example4
        example5
        example6
        example7
        )

