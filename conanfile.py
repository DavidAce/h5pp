import os
from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration

class h5ppConan(ConanFile):
    name = "h5pp"
    version = "1.10.0"
    description = "A C++17 wrapper for HDF5 with focus on simplicity"
    homepage = "https://github.com/DavidAce/h5pp"
    author = "DavidAce <aceituno@kth.se>"
    topics = ("h5pp","hdf5", "binary", "storage","conan")
    url = "https://github.com/DavidAce/h5pp"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package_multi"
    requires = "eigen/3.4.0", "spdlog/1.9.2", "fmt/8.0.1",  "hdf5/1.12.0"
    build_policy    = "missing"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    options = {
        'tests'     :[True,False],
        'examples'  :[True,False],
        'verbose'   :[True,False],
        'pch'       :[True,False],
        'ccache'    :[True,False],
        }

    default_options = {
        'tests'    : True,
        'examples' : False,
        'verbose'  : False,
        'pch'      : False,
        'ccache'   : True,
    }

    _cmake = None

    @property
    def _compilers_minimum_version(self):
        return {
            "gcc": "7.4",
            "Visual Studio": "15.7",
            "clang": "6",
            "apple-clang": "10",
        }

    def configure(self):
        if self.settings.compiler.cppstd:
            tools.check_min_cppstd(self, 17)
        minimum_version = self._compilers_minimum_version.get(str(self.settings.compiler), False)
        if minimum_version:
            if tools.Version(self.settings.compiler.version) < minimum_version:
                raise ConanInvalidConfiguration("h5pp requires C++17, which your compiler does not support.")
        else:
            self.output.warn("h5pp requires C++17. Your compiler is unknown. Assuming it supports C++17.")

    def _configure_cmake(self):
        if not self._cmake:
            self._cmake = CMake(self)
            self._cmake.definitions["H5PP_ENABLE_TESTS"]         = self.options.tests
            self._cmake.definitions["H5PP_BUILD_EXAMPLES"]       = self.options.examples
            self._cmake.definitions["H5PP_PACKAGE_MANAGER"]      = "find"
            self._cmake.definitions["H5PP_ENABLE_PCH"]           = self.options.pch
            self._cmake.definitions["H5PP_ENABLE_CCACHE"]        = self.options.ccache
            self._cmake.definitions["H5PP_ENABLE_EIGEN3"]        = True
            self._cmake.definitions["H5PP_ENABLE_SPDLOG"]        = True
            self._cmake.definitions["H5PP_ENABLE_FMT"]           = True
            args = None
            if self.options.verbose:
                args = '--loglevel=DEBUG'
                self._cmake.verbose=True
            self._cmake.configure(args=args)
        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
        if self.options.tests:
            cmake.test()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "share"))

    def package_info(self):
        self.cpp_info.names["cmake_find_package"] = "h5pp"
        self.cpp_info.names["cmake_find_package_multi"] = "h5pp"
        self.cpp_info.components["h5pp_headers"].names["cmake_find_package"] = "headers"
        self.cpp_info.components["h5pp_headers"].names["cmake_find_package_multi"] = "headers"
        self.cpp_info.components["h5pp_deps"].names["cmake_find_package"] = "deps"
        self.cpp_info.components["h5pp_deps"].names["cmake_find_package_multi"] = "deps"
        self.cpp_info.components["h5pp_deps"].requires = ["eigen::eigen", "fmt::fmt", "spdlog::spdlog", "hdf5::hdf5"]
        self.cpp_info.components["h5pp_flags"].names["cmake_find_package"] = "flags"
        self.cpp_info.components["h5pp_flags"].names["cmake_find_package_multi"] = "flags"
        self.cpp_info.components["h5pp_flags"].defines = ["H5PP_USE_EIGEN3"]
        self.cpp_info.components["h5pp_flags"].defines = ["H5PP_USE_SPDLOG"]
        self.cpp_info.components["h5pp_flags"].defines = ["H5PP_USE_FMT"]
        if self.settings.compiler == "gcc" and tools.Version(self.settings.compiler.version) < "9":
            self.cpp_info.components["h5pp_flags"].system_libs = ["stdc++fs"]
        if self.settings.compiler == "Visual Studio":
            self.cpp_info.components["h5pp_flags"].defines = ["NOMINMAX"]
            self.cpp_info.components["h5pp_flags"].cxxflags = ["/permissive-"]

    def package_id(self):
        self.info.header_only()
