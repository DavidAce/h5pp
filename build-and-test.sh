          cmake -E make_directory build/Debug
          cd build/Debug
          cmake                                                    \
          -DCMAKE_BUILD_TYPE=Debug                                 \
          -DBUILD_SHARED_LIBS:BOOL=OFF                             \
          -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON                         \
          -DH5PP_ENABLE_EIGEN3:BOOL=ON                             \
          -DH5PP_ENABLE_SPDLOG:BOOL=ON                             \
          -DH5PP_ENABLE_ASAN:BOOL=ON                               \
          -DH5PP_ENABLE_TESTS:BOOL=ON                              \
          -DH5PP_PACKAGE_MANAGER:STRING=find-or-cmake              \
          -DH5PP_PREFER_CONDA_LIBS:BOOL=OFF                        \
          -DH5PP_PRINT_INFO:BOOL=ON                                \
          -DH5PP_BUILD_EXAMPLES:BOOL=ON                            \
          -G "Unix Makefiles"                                      \
          ../../


          cd build/Debug
          cmake --build . --parallel 8

#          cd build/Debug
#          cmake --build . --target install

          cd build/Debug
          ctest -C Debug --output-on-failure