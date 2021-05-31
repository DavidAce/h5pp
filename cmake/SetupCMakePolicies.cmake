cmake_minimum_required(VERSION 3.14)

cmake_policy(SET CMP0074 NEW) # To let find_package calls use <PackageName>_ROOT variables
cmake_policy(SET CMP0075 NEW) # Include file check macros honor CMAKE_REQUIRED_LIBRARIES
cmake_policy(SET CMP0067 NEW) # try_compile honosr current CMAKE_CXX_STANDARD setting. https://stackoverflow.com/questions/47213356/cmake-using-corrext-c-standard-when-checking-for-header-files
cmake_policy(SET CMP0077 NEW) # Lets option() consider normal variables