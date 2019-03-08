


function(build_example example_name example_src_dir example_tgt_dir)
    include(ExternalProject)
    ExternalProject_Add(${example_name}
            SOURCE_DIR  ${PROJECT_SOURCE_DIR}/${example_src_dir}
            PREFIX      ${BUILD_DIRECTORY_EXAMPLES}/${example_name}
            INSTALL_DIR ${example_tgt_dir}
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
            -DH5PP_ROOT_DIR:PATH=${INSTALL_DIRECTORY_H5PP}
            DEPENDS h5pp::h5pp h5pp::deps
            EXCLUDE_FROM_ALL TRUE
            )
endfunction()


build_example(example_helloworld  examples/00_helloworld  ${INSTALL_DIRECTORY_EXAMPLES}/00_helloworld)
build_example(example_writescalar examples/01_writescalar ${INSTALL_DIRECTORY_EXAMPLES}/01_writescalar)
