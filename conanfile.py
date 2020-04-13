from conans import ConanFile, CMake


class LibrevaultConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [
        "docopt.cpp/0.6.2",
        "spdlog/1.5.0",
        "cryptopp/8.2.0@bincrafters/stable",
        "sqlite3/3.31.1",
        "boost/1.72.0",
        "protobuf/3.9.1",
        "protoc_installer/3.9.1@bincrafters/stable",
        "websocketpp/0.8.1",
        "openssl/1.1.1f",
        "qt/5.14.2@bincrafters/stable",
    ]
    generators = "cmake"
    default_options = {
        "sqlite3:threadsafe": 1,
        "sqlite3:enable_json1": True,
        "qt:qtsvg": True,
        "qt:qttranslations": True,
        "qt:qttools": True,
        "qt:qtwayland": False,
        "qt:qtwebsockets": True,
    }

    def configure(self):
        if self.settings.os == "Linux":
            self.options["qt"].qtwayland = True
