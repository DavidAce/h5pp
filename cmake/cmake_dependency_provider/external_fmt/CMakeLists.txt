cmake_minimum_required(VERSION 3.15)
project(external-fmt)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(GNUInstallDirs)
include(ExternalProject)
ExternalProject_Add(external_fmt
#        URL         https://github.com/fmtlib/fmt/archive/9.1.0.tar.gz
        URL         https://github.com/fmtlib/fmt/archive/10.1.0.tar.gz
#        URL_MD5     2914e3ac33595103d6b27c87364b034f #6.1.2
#        URL_MD5     370757a0bae552679b6481d24d768ece #7.0.0
#        URL_MD5     7d5af964c6633ef90cd6a47be3afe6a0 #8.0.1
#        URL_MD5     fe7f1585844b14c647bd332ad5562832 #8.1.1
#        URL_MD5     21fac48cae8f3b4a5783ae06b443973a #9.1.0
        URL_MD5     a2704b768020b5e02bcb72cb65be60cc #9.1.0
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
         ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_RPATH:PATH=<INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_SKIP_BUILD_RPATH:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON
        -DCMAKE_INSTALL_MESSAGE=LAZY
        # fmt flags
        -DFMT_TEST:BOOL=OFF
        -DFMT_DOC:BOOL=OFF
        )
