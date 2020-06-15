from conans import ConanFile, CMake


class LibrevaultConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [
        "docopt.cpp/0.6.2",
        "spdlog/1.5.0",
        "cryptopp/8.2.0@bincrafters/stable",
        "sqlite3/3.32.1",
        "boost/1.72.0",
        "protobuf/3.9.1",
        "protoc_installer/3.9.1@bincrafters/stable",
        "websocketpp/0.8.2",
        "openssl/1.1.1g",
        "qt/5.14.2@bincrafters/stable",
        "libjpeg/9d",  # override
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

    def requirements(self):
        if CMake.get_version() < "3.16":
            self.requires("cmake/3.16.3")
        if self.settings.os == "Windows":
            self.requires("winsparkle/0.6.0@gamepad64/stable")

    def configure(self):
        if self.settings.os == "Linux":
            self.options["qt"].qtwayland = True
        if self.settings.os == "Macos":
            self.options["qt"].qtmacextras = True
            self.options["glib"].shared = False
        if self.settings.os == "Windows":
            self.options["qt"].qtwinextras = True
