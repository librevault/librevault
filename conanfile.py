from conans import ConanFile, CMake

class LibrevaultConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = "docopt.cpp/latest@signal9/stable",\
               "spdlog/0.14.0@bincrafters/stable",\
               "sqlite3/3.18.0@jgsogo/stable"
    generators = "cmake"
    default_options = ""
