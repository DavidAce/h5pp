


include(ExternalProject)
ExternalProject_Add(external_H5PP
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/h5pp
        PREFIX      ${BUILD_DIRECTORY_H5PP}
        INSTALL_DIR ${INSTALL_DIRECTORY_H5PP}
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DMARCH=${MARCH}
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DHDF5_DIR:PATH=${HDF5_DIR}
        -DHDF5_ROOT:PATH=${HDF5_ROOT}
        -DEigen3_DIR:PATH=${Eigen3_DIR}
        -DEIGEN3_ROOT_DIR:PATH=${EIGEN3_ROOT_DIR}
        -DEIGEN3_INCLUDE_DIR:PATH=${EIGEN3_INCLUDE_DIR}
        -Dspdlog_DIR:PATH=${spdlog_DIR}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        DEPENDS hdf5 Eigen3 spdlog
        EXCLUDE_FROM_ALL TRUE
        )
ExternalProject_Get_Property(external_H5PP INSTALL_DIR)
add_library(h5pp INTERFACE)
add_dependencies(h5pp external_H5PP)
#target_link_libraries(
#        h5pp
#        INTERFACE
#        ${INSTALL_DIR}/lib/libh5pp.a
#)
target_include_directories(
        h5pp
        INTERFACE
        $<BUILD_INTERFACE:${INSTALL_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(h5pp INTERFACE cxx_std_17)
target_compile_options(h5pp INTERFACE -std=c++17)
target_link_libraries(h5pp INTERFACE -lstdc++fs)

