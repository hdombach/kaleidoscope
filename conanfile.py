from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import cmake_layout

class MainRecipe(ConanFile):
    generators = "CMakeToolchain", "CMakeDeps"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("stb/cci.20230920")
        self.requires("glfw/3.4")
        self.requires("tinyobjloader/2.0.0-rc10")
        self.requires("glm/cci.20230113")
        self.requires("imgui/1.91.4-docking")
        self.requires("vulkan-headers/1.3.268.0")
        self.requires("shaderc/2023.6")
        self.requires("portable-file-dialogs/0.1.0")


    def generate(self):
        for dep in self.dependencies.values():
            for srcdir in dep.cpp_info.srcdirs:
                copy(self, "*", srcdir, self.build_folder)
