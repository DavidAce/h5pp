from conans import ConanFile, CMake, tools
from conans.tools import download, unzip
import os, re

class h5ppConan(ConanFile):
    name = "h5pp"
    version = "1.7.4"
    author = "DavidAce <aceituno@kth.se>"
    topics = ("hdf5", "binary", "storage")
    url = "https://github.com/DavidAce/h5pp"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    requires = "eigen/3.3.7@conan/stable", "spdlog/1.4.2@bincrafters/stable", "hdf5/1.10.5"
    build_policy    = "missing"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    options = {
        'shared'    :[True,False],
        'tests'     :[True,False],
        'examples'  :[True,False],
        'verbose'   :[True,False],
        }
    default_options = (
        'shared=False',
        'tests=True',
        'examples=False',
        'verbose=False',
    )

    def configure(self):
        tools.check_min_cppstd(self, "17")


    def build(self):
        cmake = CMake(self)
        cmake.definitions["H5PP_ENABLE_TESTS"]         = self.options.tests
        cmake.definitions["H5PP_BUILD_EXAMPLES"]       = self.options.examples
        cmake.definitions["H5PP_PRINT_INFO"]           = self.options.verbose
        cmake.definitions["H5PP_DOWNLOAD_METHOD"]      = "conan"
        cmake.configure()
        cmake.build()
        cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_id(self):
        self.info.header_only()
