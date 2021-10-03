from conans import ConanFile, CMake


class LibrevaultConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [
        "docopt.cpp/0.6.2",
        "spdlog/1.9.2",
        "cryptopp/8.5.0",
        "sqlite3/3.36.0",
        "boost/1.76.0",
        "protobuf/3.17.1",
        "websocketpp/0.8.2",
        "openssl/1.1.1k",
        "qt/5.15.2",
    ]
    generators = ["cmake_find_package", "cmake"]
    default_options = {
        "sqlite3:threadsafe": 1,
        "qt:shared": True,
        "qt:qtsvg": True,
        "qt:qttranslations": True,
        "qt:qttools": True,
        # "qt:qtwayland": False,
        "qt:qtwebsockets": True,
        "qt:with_odbc": False,
        "qt:with_sqlite3": False,
        "qt:with_mysql": False,
        "qt:with_pq": False,
    }

    # def requirements(self):
        # if CMake.get_version() < "3.16":
        #     self.requires("cmake/3.16.3")
        # if self.settings.os == "Windows":
        #     self.requires("winsparkle/0.6.0@gamepad64/stable")

    def configure(self):
        # if self.settings.os == "Linux":
        #     self.options["qt"].qtwayland = True
        #     if self.settings.build_type == "Debug":
        #         self.options["qt"].with_vulkan = True
        if self.settings.os == "Macos":
            self.options["qt"].qtmacextras = True
            self.options["glib"].shared = False
        if self.settings.os == "Windows":
            self.options["qt"].qtwinextras = True
