from conan import ConanFile

class ManageMySelfConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("sqlitecpp/3.3.1")