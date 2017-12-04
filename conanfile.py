from conans import ConanFile, CMake

class LibrevaultConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = "docopt.cpp/latest@signal9/stable",\
               "spdlog/0.14.0@bincrafters/stable",\
               "cryptopp/5.6.5@bincrafters/stable",\
               "sqlite3/3.21.0@bincrafters/stable"
    generators = "cmake"
    default_options = ""
