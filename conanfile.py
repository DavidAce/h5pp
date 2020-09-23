import os
from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration

class h5ppConan(ConanFile):
    name = "h5pp"
    version = "1.8.4"
    description = "A C++17 wrapper for HDF5 with focus on simplicity"
    homepage = "https://github.com/DavidAce/h5pp"
    author = "DavidAce <aceituno@kth.se>"
    topics = ("h5pp","hdf5", "binary", "storage","conan")
    url = "https://github.com/DavidAce/h5pp"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "cmake_find_package"
    requires = "eigen/3.3.7", "spdlog/1.8.0", "hdf5/1.12.0"
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
        }

    default_options = {
        'tests'    : True,
        'examples' : False,
        'verbose'  : False,
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
            self._cmake.definitions["H5PP_PRINT_INFO"]           = self.options.verbose
            self._cmake.definitions["H5PP_DOWNLOAD_METHOD"]      = "conan"
            self._cmake.configure()
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
        if self.settings.compiler == "gcc" and tools.Version(self.settings.compiler.version) < "9":
            self.cpp_info.system_libs = ["stdc++fs"]
        if self.settings.compiler == "Visual Studio":
            self.cpp_info.defines = ["NOMINMAX"]
            self.cpp_info.cxxflags = ["/permissive-"]
        self.cpp_info.names["cmake_find_package"] = "h5pp"
        self.cpp_info.names["cmake_find_package_multi"] = "h5pp"

    def package_id(self):
        self.info.header_only()
