from conans import ConanFile, CMake, tools
from conans.tools import download, unzip
import os, re

class h5ppConan(ConanFile):
    name = "h5pp"
    # version = "1.7.0"
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
        "subfolder": "h5pp",
        "url": "auto",
        "revision": "auto"
    }

    options         = {
        'shared': [True, False],
    }
    default_options = (
        'shared=False',
    )

    # def source(self):
    #     zip_name = self.version+".zip"
    #     download("https://github.com/DavidAce/h5pp/archive/v"+self.version+".zip", zip_name)
    #     unzip(zip_name)
    #     git = tools.Git(folder="h5pp-"+self.version)
    #     # git.clone("https://github.com/DavidAce/h5pp.git", "master")

    def configure(self):
        tools.check_min_cppstd(self, "17")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["H5PP_ENABLE_TESTS:BOOL"] = True
        cmake.definitions["H5PP_BUILD_EXAMPLES:BOOL"] = False
        cmake.definitions["H5PP_PRINT_INFO:BOOL"] = True
        cmake.definitions["H5PP_DOWNLOAD_METHOD:STRING"] = "conan"
        if tools.os_info.is_linux:
            cmake.definitions['BUILD_SHARED_LIBS:BOOL'] = True if self.options.shared else False

        cmake.configure(source_folder=self.build_folder + '/h5pp-' + self.version)
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        cmake.test()

    def package_id(self):
        self.info.header_only()
