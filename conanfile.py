from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import copy
from conan.tools.microsoft import is_msvc
from conan.tools.build import check_min_cppstd
from conan.tools.scm import Version
from conan.errors import ConanInvalidConfiguration
import os

required_conan_version = ">=1.45.0"

class H5ppConan(ConanFile):
    name = "h5pp"
    version = "1.11.1"
    description = "A C++17 wrapper for HDF5 with focus on simplicity"
    homepage = "https://github.com/DavidAce/h5pp"
    author = "DavidAce <aceituno@kth.se>"
    topics = ("h5pp", "hdf5", "binary", "storage", "header-only", "cpp17")
    url = "https://github.com/DavidAce/h5pp"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"
    no_copy_source = True
    short_paths = True
    options = {
        "with_eigen": [True, False],
        "with_spdlog": [True, False],
    }
    default_options = {
        "with_eigen": True,
        "with_spdlog": True,
    }

    @property
    def _compilers_minimum_version(self):
        return {
            "gcc": "7.4",
            "Visual Studio": "15.7",
            "clang": "6",
            "apple-clang": "10",
        }

    def requirements(self):
        self.requires("hdf5/1.14.0", transitive_headers=True, transitive_libs=True)
        if self.options.get_safe('with_eigen'):
            self.requires("eigen/3.4.0", transitive_headers=True)
        if self.options.get_safe('with_spdlog'):
            self.requires("spdlog/1.11.0", transitive_headers=True, transitive_libs=True)

    def layout(self):
        basic_layout(self)

    def package_id(self):
        self.info.clear()

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, 17)
        minimum_version = self._compilers_minimum_version.get(str(self.settings.compiler), False)
        if minimum_version:
            if Version(self.settings.compiler.version) < minimum_version:
                raise ConanInvalidConfiguration("h5pp requires C++17, which your compiler does not support.")
        else:
            self.output.warn("h5pp requires C++17. Your compiler is unknown. Assuming it supports C++17.")

    def package(self):
        copy(self, pattern="*", src=os.path.join(self.source_folder, "include"), dst=os.path.join(self.package_folder, "include"))
        copy(self, pattern="LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))


    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "h5pp")
        self.cpp_info.set_property("cmake_target_name", "h5pp::h5pp")
        self.cpp_info.components["h5pp_headers"].set_property("cmake_target_name", "h5pp::headers")
        self.cpp_info.components["h5pp_flags"].set_property("cmake_target_name", "h5pp::flags")
        self.cpp_info.components["h5pp_deps"].set_property("cmake_target_name", "h5pp::deps")
        self.cpp_info.components["h5pp_deps"].requires = ["hdf5::hdf5"]

        if self.options.with_eigen:
            self.cpp_info.components["h5pp_deps"].requires.append("eigen::eigen")
            self.cpp_info.components["h5pp_flags"].defines.append("H5PP_USE_EIGEN3")
        if self.options.with_spdlog:
            self.cpp_info.components["h5pp_deps"].requires.append("spdlog::spdlog")
            self.cpp_info.components["h5pp_flags"].defines.append("H5PP_USE_SPDLOG")
            self.cpp_info.components["h5pp_flags"].defines.append("H5PP_USE_FMT")

        if (self.settings.compiler == "gcc" and Version(self.settings.compiler.version) < "9") or \
           (self.settings.compiler == "clang" and self.settings.compiler.get_safe("libcxx") in ["libstdc++", "libstdc++11"]):
            self.cpp_info.components["h5pp_flags"].system_libs = ["stdc++fs"]
        if is_msvc(self):
            self.cpp_info.components["h5pp_flags"].defines.append("NOMINMAX")
            self.cpp_info.components["h5pp_flags"].cxxflags = ["/permissive-"]

        # TODO: to remove in conan v2 once cmake_find_package_* generators removed
        self.cpp_info.names["cmake_find_package"] = "h5pp"
        self.cpp_info.names["cmake_find_package_multi"] = "h5pp"
        self.cpp_info.components["h5pp_headers"].names["cmake_find_package"] = "headers"
        self.cpp_info.components["h5pp_headers"].names["cmake_find_package_multi"] = "headers"
        self.cpp_info.components["h5pp_deps"].names["cmake_find_package"] = "deps"
        self.cpp_info.components["h5pp_deps"].names["cmake_find_package_multi"] = "deps"
        self.cpp_info.components["h5pp_flags"].names["cmake_find_package"] = "flags"
        self.cpp_info.components["h5pp_flags"].names["cmake_find_package_multi"] = "flags"