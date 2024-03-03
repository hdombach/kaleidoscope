from conan import ConanFile
from conan.tools.files import copy

class MainRecipe(ConanFile):
    generators = "CMakeToolchain", "CMakeDeps"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("stb/cci.20230920")
        self.requires("glfw/3.4")
        self.requires("tinyobjloader/2.0.0-rc10")
        self.requires("glm/cci.20230113")
        self.requires("imgui/cci.20230105+1.89.2.docking")
        self.requires("vulkan-headers/1.3.268.0")


    def generate(self):
        for dep in self.dependencies.values():
            for srcdir in dep.cpp_info.srcdirs:
                copy(self, "*", srcdir, self.build_folder)
