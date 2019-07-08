
include(GNUInstallDirs)
message(STATUS "Fetch spdlog given directory spdlog_DIR: ${spdlog_DIR}")
find_package(spdlog 1.3 NO_DEFAULT_PATH PATHS ${H5PP_INSTALL_DIR_THIRD_PARTY}/spdlog/${CMAKE_INSTALL_LIBDIR}/cmake/spdlog ${spdlog_DIR} )

if(spdlog_FOUND)
    add_library(spdlog INTERFACE)
    get_target_property(SPDLOG_INCLUDE_DIR spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(spdlog INTERFACE ${SPDLOG_INCLUDE_DIR})
    message(STATUS "SPDLOG FOUND IN SYSTEM: ${SPDLOG_INCLUDE_DIR}")

elseif (DOWNLOAD_SPDLOG OR DOWNLOAD_ALL)
    message(STATUS "Spdlog will be installed into ${H5PP_INSTALL_DIR_THIRD_PARTY}/spdlog on first build.")
    include(ExternalProject)
    ExternalProject_Add(external_SPDLOG
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.3.1
            GIT_PROGRESS 1
            UPDATE_COMMAND ""
            TEST_COMMAND ""
            PREFIX      ${H5PP_BUILD_DIR_THIRD_PARTY}/spdlog
            INSTALL_DIR ${H5PP_INSTALL_DIR_THIRD_PARTY}/spdlog
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
            -DSPDLOG_BUILD_EXAMPLES:BOOL=OFF
            -DSPDLOG_BUILD_BENCH:BOOL=OFF
            -DSPDLOG_BUILD_TESTS:BOOL=OFF
            )


    ExternalProject_Get_Property(external_SPDLOG INSTALL_DIR)
    add_library(spdlog INTERFACE)
    add_library(spdlog::spdlog ALIAS spdlog)
    set(spdlog_DIR ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/cmake/spdlog)
    add_dependencies(spdlog external_SPDLOG)

    target_include_directories(
            spdlog
            INTERFACE
            $<BUILD_INTERFACE:${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR}>
            $<INSTALL_INTERFACE:third-party/spdlog/${CMAKE_INSTALL_INCLUDEDIR}>
    )
else()
    message(STATUS "Dependency spdlog not found and DOWNLOAD_SPDLOG is OFF")

endif()