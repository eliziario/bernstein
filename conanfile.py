from conan import ConanFile
from conan.tools.cmake import cmake_layout


class BernsteinConan(ConanFile):
    name = "bernstein"
    version = "0.1.0"
    package_type = "application"

    settings = "os", "compiler", "build_type", "arch"

    requires = (
        "cli11/2.4.2",
        "fmt/11.1.3",
        "nlohmann_json/3.11.3",
        "spdlog/1.15.1",
    )
    test_requires = ("catch2/3.7.1",)

    generators = "CMakeDeps", "CMakeToolchain"

    def layout(self):
        cmake_layout(self)
