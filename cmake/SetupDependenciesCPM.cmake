
if(H5PP_PACKAGE_MANAGER MATCHES "cpm")
    include(cmake/CPM.cmake)
    if(H5PP_ENABLE_EIGEN3)
        CPMAddPackage(
                NAME eigen
                GITLAB_REPOSITORY libeigen/eigen
                GIT_TAG 3.4.0
                OPTIONS
                "EIGEN_TEST_CXX11 OFF"
                "BUILD_TESTING OFF"
                "EIGEN_BUILD_DOC OFF"
                "EIGEN_BUILD_PKGCONFIG OFF"
        )
#        CPMAddPackage("gl:libeigen/eigen#3.4-rc1"
#                EXCLUDE_FROM_ALL TRUE)
        if(TARGET eigen)
            target_link_libraries(deps INTERFACE eigen)
            if(NOT TARGET Eigen3::Eigen)
                add_library(Eigen3::Eigen ALIAS eigen)
            endif()
        else()
            message(FATAL_ERROR "CPM failed to add package: Eigen")
        endif()
    endif()
    if(H5PP_ENABLE_FMT)
        CPMAddPackage(NAME fmt
                GITHUB_REPOSITORY fmtlib/fmt
                GIT_TAG 8.0.1
                OPTIONS "FMT_INSTALL ON" )
        find_package(fmt REQUIRED)
        if(TARGET fmt)
            target_link_libraries(deps INTERFACE fmt)
            if(NOT TARGET fmt::fmt)
                add_library(fmt::fmt ALIAS fmt)
            endif()
        else()
            message(FATAL_ERROR "CPM failed to add package: fmt")
        endif()
    endif()
    if(H5PP_ENABLE_SPDLOG)
        CPMAddPackage(
                NAME spdlog
                GITHUB_REPOSITORY gabime/spdlog
                VERSION 1.9.2
                OPTIONS
                "SPDLOG_INSTALL ON"
                "SPDLOG_BUILD_SHARED ${BUILD_SHARED_LIBS}"
                "SPDLOG_FMT_EXTERNAL ON"
                "SPDLOG_FMT_EXTERNAL_HO OFF"
                "SPDLOG_ENABLE_PCH ${H5PP_ENABLE_PCH}"
                "SPDLOG_BUILD_EXAMPLE OFF"
                "SPDLOG_BUILD_EXAMPLE_HO OFF"
                "SPDLOG_BUILD_TESTS OFF"
                "SPDLOG_BUILD_TESTS_HO OFF"
        )
        if(TARGET spdlog)
            target_link_libraries(deps INTERFACE spdlog)
            if(NOT TARGET spdlog::spdlog)
                add_library(spdlog::spdlog ALIAS spdlog)
            endif()
        else()
            message(FATAL_ERROR "CPM failed to add package: spdlog")
        endif()
    endif()

    # hdf5 does not support add_subdirectory, so we roll or own installer
    include(cmake/InstallHDF5.cmake)
    install_hdf5()
    target_link_libraries(deps INTERFACE ${hdf5_TARGET})

endif()