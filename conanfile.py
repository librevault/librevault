from conans import ConanFile, CMake


class LibrevaultConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [
        "docopt.cpp/0.6.2",
        "spdlog/0.14.0@bincrafters/stable",
        "cryptopp/8.2.0@bincrafters/stable",
        "sqlite3/3.31.1",
        "boost/1.72.0",
        "protobuf/3.9.1",
        "protoc_installer/3.9.1@bincrafters/stable",
        "websocketpp/0.8.1",
    ]
    generators = "cmake"
    default_options = {
        "sqlite3:threadsafe": 1
    }
