


#message("HDF5_ROOT:PATH=${HDF5_ROOT}")
#message("HDF5_DIR:PATH=${HDF5_DIR}")
#message("Eigen3_DIR:PATH=${Eigen3_DIR}")
#message("EIGEN3_ROOT_DIR:PATH=${EIGEN3_ROOT_DIR}")
#message("EIGEN3_INCLUDE_DIR:PATH=${EIGEN3_INCLUDE_DIR}")
#message("spdlog_DIR:PATH=${spdlog_DIR}")



include(ExternalProject)
ExternalProject_Add(external_H5PP
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/h5pp
        PREFIX      ${BUILD_DIRECTORY}/h5pp
        INSTALL_DIR ${INSTALL_DIRECTORY}/h5pp
#        INSTALL_COMMAND ""
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DHDF5_ROOT:PATH=${HDF5_ROOT}
        -DHDF5_DIR:PATH=${HDF5_DIR}
        -DEigen3_DIR:PATH=${Eigen3_DIR}
        -DEIGEN3_ROOT_DIR:PATH=${EIGEN3_ROOT_DIR}
        -DEIGEN3_INCLUDE_DIR:PATH=${EIGEN3_INCLUDE_DIR}
        -Dspdlog_DIR:PATH=${spdlog_DIR}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        DEPENDS hdf5 Eigen3 spdlog
        )



ExternalProject_Get_Property(external_H5PP INSTALL_DIR)
add_library(h5pp INTERFACE)
#add_library(h5pp::h5pp ALIAS h5pp)
#set(h5pp_DIR ${INSTALL_DIR}/lib/cmake/h5pp)
#set(h5pp_ROOT ${INSTALL_DIR})
add_dependencies(h5pp external_H5PP)
target_link_libraries(
        h5pp
        INTERFACE
        ${INSTALL_DIR}/lib/libh5pp.a
)
target_include_directories(
        h5pp
        INTERFACE
        $<BUILD_INTERFACE:${INSTALL_DIR}/include>
        $<INSTALL_INTERFACE:h5pp/include>
)
target_compile_features(h5pp INTERFACE cxx_std_17)
target_compile_options(h5pp INTERFACE -std=c++17)
target_link_libraries(h5pp INTERFACE -lstdc++fs)

