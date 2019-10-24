function(build_third_party library_name config_dir build_dir install_dir extra_flags)
#    set(extra_flags)
#    foreach(flag ${extra_flags_nonparsed})
#        list(APPEND extra_flags ${flag})
#    endforeach()
    message(STATUS "library_name      : ${library_name}")
    message(STATUS "config_dir        : ${config_dir}")
    message(STATUS "build_dir         : ${build_dir}")
    message(STATUS "install_dir       : ${install_dir}")
    message(STATUS "extra_flags       : ${extra_flags}")
    message(STATUS "CMAKE_SOURCE_DIR  : ${CMAKE_SOURCE_DIR}")
    execute_process( COMMAND  ${CMAKE_COMMAND} -E make_directory ${config_dir}/${library_name})
    execute_process(
            COMMAND  ${CMAKE_COMMAND}
            -DBUILD_DIR:PATH=${build_dir}
            -DINSTALL_DIR:PATH=${install_dir}
            ${extra_flags}
            -G "CodeBlocks - Unix Makefiles"
            ${CMAKE_SOURCE_DIR}/cmake-modules/external_${library_name}
            WORKING_DIRECTORY ${config_dir}/${library_name}
            RESULT_VARIABLE config_result
    )

    execute_process(COMMAND  ${CMAKE_COMMAND} --build . --target all
            WORKING_DIRECTORY "${config_dir}/${library_name}"
            RESULT_VARIABLE build_result
    )

    set(config_result ${config_result} PARENT_SCOPE)
    set(build_result ${build_result} PARENT_SCOPE)
endfunction()