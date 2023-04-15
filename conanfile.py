from conan import ConanFile
from conan.tools.files import copy

class MainRecipe(ConanFile):
    generators = "CMakeToolchain", "CMakeDeps"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("stb/cci.20220909")
        self.requires("glfw/3.3.8")
        self.requires("tinyobjloader/1.0.7")
        self.requires("glm/cci.20230113")
        self.requires("imgui/cci.20230105+1.89.2.docking")

    def generate(self):
        for dep in self.dependencies.values():
            for srcdir in dep.cpp_info.srcdirs:
                copy(self, "*", srcdir, self.build_folder)
