Build notes
===========

Dependencies
------------

These dependencies are required:

| Library      | Minimum version |
|--------------|-----------------|
| GCC or Clang | 4.9, 3.4        |
| CMake        | 3.2             |
| Boost        | 1.58.0          |
| OpenSSL      | 1.0.1           |
| Crypto++     | 5.6.2           |
| Qt5          | 5.3             |
| Protobuf     | 3.0             |

Other dependencies are listed in [dependencies.md](dependencies.md). Most of them are included in the source tree as submodules or as amalgamated sources (jsoncpp and sqlite).

Building from source
--------------------

After all the dependencies are built and installed, invoke these commands in source directory, cloned from Git or unpacked from [tarball](https://releases.librevault.com/sources/librevault_latest.tar.gz):  
**Note**: If you are the cloning source code tree from Git, make sure, that you have cloned all the submodules by command `git submodule update --init`

```
mkdir build && cd build
cmake .. && cmake --build .
```

### Installing
You can perform installation using this command: 
```
sudo make install (or sudo checkinstall)
```

Dependency build instructions: Ubuntu & Debian
----------------------------------------------

* On at least Ubuntu 15.10+ and Debian 7+ you can install most of the dependencies by using this command:

```
sudo apt install build-essential cmake libboost-all-dev libssl-dev qtbase5-dev libqt5svg5-dev libqt5websockets5-dev
```

* If `libcrypto++-dev` version is < 5.2, then you should [download](http://www.cryptopp.com/#download), unpack and build it yourself:

```
sudo make install (or sudo checkinstall)
```

* Then, [download](https://github.com/google/protobuf/releases) Protobuf 3.0 source code (it has no Ubuntu or Debian packages now). Then, unpack and build it:

```
./autogen.sh && \
./configure && \
make && \
sudo make install (or sudo checkinstall)
```

### Ubuntu 16.10

All dependencies are available from the repository. Simply install them using the following command:

```
sudo apt install build-essential cmake libboost-all-dev libssl-dev qtbase5-dev libqt5svg5-dev libqt5websockets5-dev libcrypto++-dev qttools5-dev-tools protobuf-compiler libsqlite3-dev
```
