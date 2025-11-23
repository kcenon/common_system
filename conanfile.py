from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import copy
import os


class CommonSystemConan(ConanFile):
    name = "common_system"
    version = "1.0.0"
    license = "BSD-3-Clause"
    author = "kcenon"
    url = "https://github.com/kcenon/common_system"
    description = "Header-only C++20 common utilities library with Result<T>, EventBus, ObjectPool, and more"
    topics = ("cpp20", "header-only", "result-type", "event-bus", "utilities")

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "header_only": [True, False],
    }
    default_options = {
        "header_only": True,
    }

    exports_sources = "include/*", "CMakeLists.txt", "src/*", "cmake/*"
    no_copy_source = True

    def validate(self):
        from conan.tools.build import check_min_cppstd
        check_min_cppstd(self, "20")

    def layout(self):
        cmake_layout(self, src_folder=".")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["COMMON_BUILD_TESTS"] = False
        tc.variables["COMMON_BUILD_INTEGRATION_TESTS"] = False
        tc.variables["COMMON_BUILD_EXAMPLES"] = False
        tc.variables["COMMON_BUILD_DOCS"] = False
        tc.variables["COMMON_HEADER_ONLY"] = self.options.header_only
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        # Copy license
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

        # Copy headers
        copy(self, "*.h", src=os.path.join(self.source_folder, "include"),
             dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.hpp", src=os.path.join(self.source_folder, "include"),
             dst=os.path.join(self.package_folder, "include"))

        # Copy generated headers from build folder
        copy(self, "*.h", src=os.path.join(self.build_folder, "include"),
             dst=os.path.join(self.package_folder, "include"))

        # For non-header-only builds, copy libraries
        if not self.options.header_only:
            copy(self, "*.a", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
            copy(self, "*.lib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "common_system")
        self.cpp_info.set_property("cmake_target_name", "kcenon::common")

        self.cpp_info.includedirs = ["include"]

        # Header-only library, no libs needed
        if self.options.header_only:
            self.cpp_info.bindirs = []
            self.cpp_info.libdirs = []

        # C++20 required
        self.cpp_info.cxxflags = []

        # Define EVENT_BUS_ABI_VERSION
        self.cpp_info.defines = ["EVENT_BUS_ABI_VERSION=1"]

    def package_id(self):
        # Header-only: package is the same for all settings
        if self.options.header_only:
            self.info.clear()
