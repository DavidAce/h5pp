import os
from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration
from conans.errors import ConanException
from conans.tools import Version

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
    requires = "hdf5/1.12.1"
    build_policy    = "missing"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto",
        "subfolder": "source_subfolder"}

    options = {"with_eigen": [True, False],
               "with_spdlog": [True, False],
               "with_fmt":  [True, False]}

    default_options = {"with_eigen": True,
                       "with_spdlog": True,
                       "with_fmt"   : True}

    _cmake = None

    @property
    def _source_subfolder(self):
        return "source_subfolder"

    @property
    def _compilers_minimum_version(self):
        return {
            "gcc": "7.4",
            "Visual Studio": "15.7",
            "clang": "6",
            "apple-clang": "10",
        }

    def requirements(self):
        if self.options.with_eigen:
            self.requires("eigen/3.4.0")
        if self.options.with_spdlog:
            self.requires("spdlog/1.9.2")
        if self.options.with_fmt:
            self.requires("fmt/8.0.1")

    def _configure_cmake(self):
        if not self._cmake:
            self._cmake = CMake(self)
            if Version(self.version) >= "1.9.0":
                self._cmake.definitions["H5PP_PACKAGE_MANAGER"]  = "none"
            self._cmake.configure(source_folder=self._source_subfolder)
        return self._cmake

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            tools.check_min_cppstd(self, 17)

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        print("Copying license")
        self.copy("LICENSE", src=self._source_subfolder, dst="licenses")
        if Version(self.version) <= "1.8.6":
            tools.rmdir(os.path.join(self.package_folder, "share"))
            self.output.success("package(): Removed directory: share")
        else:
            tools.rmdir(os.path.join(self.package_folder, "lib/cmake"))
            self.output.success("package(): Removed directory: lib/cmake")


    def package_id(self):
        self.info.header_only()

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "h5pp")
        self.cpp_info.components["h5pp_headers"].set_property("cmake_target_name", "headers")
        self.cpp_info.components["h5pp_deps"].set_property("cmake_target_name", "deps")
        self.cpp_info.components["h5pp_flags"].set_property("cmake_target_name", "flags")

        self.cpp_info.components["h5pp_deps"].requires = ["eigen::eigen", "fmt::fmt", "spdlog::spdlog", "hdf5::hdf5"]
        self.cpp_info.components["h5pp_headers"].includedirs = ["include"]
        self.cpp_info.components["h5pp_headers"].requires = ["h5pp_deps",  "h5pp_flags"]

        if self.options.with_eigen:
            self.cpp_info.components["h5pp_flags"].defines.append("H5PP_USE_EIGEN3")
        if self.options.with_spdlog:
            self.cpp_info.components["h5pp_flags"].defines.append("H5PP_USE_SPDLOG")
        if self.options.with_fmt:
            self.cpp_info.components["h5pp_flags"].defines.append("H5PP_USE_FMT")

        if self.settings.compiler == "gcc" and tools.Version(self.settings.compiler.version) < "9":
            self.cpp_info.components["h5pp_flags"].system_libs = ["stdc++fs"]
        if self.settings.compiler == "Visual Studio":
            self.cpp_info.components["h5pp_flags"].defines.append("NOMINMAX")
            self.cpp_info.components["h5pp_flags"].cxxflags = ["/permissive-", "/EHsc"]
