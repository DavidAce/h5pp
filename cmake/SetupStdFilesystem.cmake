# h5pp requires the filesystem header (and possibly stdc++fs library)
find_package(Filesystem COMPONENTS Final Experimental)
if (TARGET std::filesystem)
    target_link_libraries(deps INTERFACE std::filesystem)
elseif(H5PP_PACKAGE_MANAGER MATCHES "cmake|fetch|cpm|conan")
    message(STATUS "Your compiler lacks std::filesystem. A drop-in replacement 'ghc::filesystem' will be downloaded")
    message(STATUS "Read more about ghc::filesystem here: https://github.com/gulrak/filesystem")
    include(cmake/InstallPackage.cmake)
    if(H5PP_PREFIX_ADD_PKGNAME)
        set(INSTALL_PREFIX_PKGNAME INSTALL_PREFIX_PKGNAME)
    endif()
    install_package(ghc_filesystem
            FIND_NAME ghc_filesystem
            TARGET_NAME ghcFilesystem::ghc_filesystem
            TARGET_HINTS ghcFilesystem::ghc_filesystem
            ${INSTALL_PREFIX_PKGNAME})

    target_link_libraries(deps INTERFACE ghcFilesystem::ghc_filesystem)
else()
    message(STATUS "Your compiler lacks std::filesystem. Set H5PP_PACKAGE_MANAGER to 'cmake', 'fetch', 'cpm' or 'conan' to get the ghc::filesystem replacement")
    message(STATUS "Read more about ghc::filesystem here: https://github.com/gulrak/filesystem")
    message(FATAL_ERROR "<filesystem> header and/or library not found")
endif()