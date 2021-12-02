# h5pp requires the filesystem header (and possibly stdc++fs library)
find_package(Filesystem COMPONENTS Final Experimental REQUIRED)
target_link_libraries(deps INTERFACE std::filesystem)
