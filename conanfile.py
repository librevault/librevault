from conans import ConanFile, CMake

class PocoTimerConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = "docopt.cpp/latest@signal9/stable",\
               "spdlog/latest@signal9/stable",\
               #"sqlite3/3.18.0@jgsogo/stable"
    generators = "cmake"
    default_options = "Poco:shared=True", "OpenSSL:shared=True"
