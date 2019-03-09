


function(build_example example_name example_src_dir example_tgt_dir)
    include(ExternalProject)
    ExternalProject_Add(${example_name}
            SOURCE_DIR  ${PROJECT_SOURCE_DIR}/${example_src_dir}
            PREFIX      ${H5PP_BUILD_DIR_EXAMPLES}/${example_name}
            INSTALL_DIR ${example_tgt_dir}
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
            -DH5PP_INSTALL_DIR:PATH=${H5PP_INSTALL_DIR}
            DEPENDS h5pp::h5pp h5pp::deps
            EXCLUDE_FROM_ALL TRUE
            )
endfunction()


build_example(example_helloworld  h5pp/examples/00_helloworld  ${H5PP_INSTALL_DIR_EXAMPLES}/00_helloworld)
build_example(example_writescalar h5pp/examples/01_writescalar ${H5PP_INSTALL_DIR_EXAMPLES}/01_writescalar)
